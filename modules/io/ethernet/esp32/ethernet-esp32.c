/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "builtinCommon.h"

#include "esp_eth.h"
#include "esp_eth_driver.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "lwip/ip6_addr.h"

#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
#include "ethernet.h"
#include "modSPI.h"
#endif

#ifndef MODDEF_ETHERNET_SPI_COMMAND_BITS
#define MODDEF_ETHERNET_SPI_COMMAND_BITS (3)
#endif
#ifndef MODDEF_ETHERNET_SPI_ADDRESS_BITS
#define MODDEF_ETHERNET_SPI_ADDRESS_BITS (5)
#endif
#ifndef MODDEF_ETHERNET_HZ
#define MODDEF_ETHERNET_HZ (16000000)
#endif

typedef struct xsEthernetRecord *xsEthernet;
typedef struct xsEthernetRecord {
	xsEthernet	next;
	xsSlot		obj;
	xsMachine	*the;

	uint32_t	useCount;
	uint8_t		connected:1;
	uint8_t		connecting:1;
	uint8_t		closed:1;
	uint8_t		ip;

	xsSlot		*onChanged;
} xsEthernetRecord;

static esp_netif_t *gNetif;
static xsEthernet gEthernetList;
static uint8_t gIP;
static uint8_t gConnected;

static void doEthernetEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void doIPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void ethernetConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen);
static void initEthernet(void);

typedef struct {
	int32_t		event_id;
	uint8_t		ipInit;
} EthernetEventMsg;

static void formatMAC(const uint8_t *mac, char *str)
{
	static const char hex[] = "0123456789abcdef";
	char *s = str;
	for (int i = 0; i < 6; i++) {
		if (i) *s++ = ':';
		*s++ = hex[(mac[i] >> 4) & 0x0F];
		*s++ = hex[mac[i] & 0x0F];
	}
	*s = 0;
}

void xs_ethernet_destructor(void *data)
{
	xsEthernet eth = (xsEthernet)data;
	if (!eth) return;

	if (__atomic_sub_fetch(&eth->useCount, 1, __ATOMIC_SEQ_CST) > 0)
		return;

	c_free(eth);
}

static void xs_ethernet_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	xsEthernet eth = (xsEthernet)it;

	if (eth->onChanged)
		(*markRoot)(the, eth->onChanged);
}

static const xsHostHooks xsEthernetHooks = {
	xs_ethernet_destructor,
	xs_ethernet_mark,
	C_NULL
};

void xs_ethernet(xsMachine *the)
{
	xsSlot *onChanged = builtinGetCallback(the, xsID_onChanged);

	xsEthernet eth = c_calloc(1, sizeof(xsEthernetRecord));
	if (!eth)
		xsUnknownError("no memory");

	eth->the = the;
	eth->obj = xsThis;
	__atomic_store_n(&eth->useCount, 1, __ATOMIC_SEQ_CST);
	eth->onChanged = onChanged;

	xsmcSetHostData(xsThis, eth);
	xsSetHostHooks(xsThis, &xsEthernetHooks);
	xsRemember(eth->obj);

	initEthernet();

	esp_netif_ip_info_t ip_info;
	if (ESP_OK == esp_netif_get_ip_info(gNetif, &ip_info) && ip_info.ip.addr) {
		eth->connected = 1;
		eth->ip |= 0x01;
	}

	esp_ip6_addr_t ip6;
	if (ESP_OK == esp_netif_get_ip6_linklocal(gNetif, &ip6))
		eth->ip |= 0x02;

	if (eth->ip) {
		gIP = eth->ip;
		gConnected = 1;
	}
	else if (gConnected)
		eth->connected = 1;

	eth->next = gEthernetList;
	gEthernetList = eth;
}

void xs_ethernet_close(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostData(xsThis);
	if (!eth) return;
	xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	eth->closed = 1;
	xsForget(eth->obj);

	if (gEthernetList == eth)
		gEthernetList = eth->next;
	else {
		for (xsEthernet walker = gEthernetList; walker; walker = walker->next) {
			if (walker->next == eth) {
				walker->next = eth->next;
				break;
			}
		}
	}

	xs_ethernet_destructor(eth);
	xsmcSetHostData(xsThis, C_NULL);
	xsSetHostDestructor(xsThis, C_NULL);
}

#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
static void init_spi(void)
{
#ifdef MODDEF_ETHERNET_SPI_MISO_PIN
	spi_bus_config_t buscfg = {
		.miso_io_num = MODDEF_ETHERNET_SPI_MISO_PIN,
		.mosi_io_num = MODDEF_ETHERNET_SPI_MOSI_PIN,
		.sclk_io_num = MODDEF_ETHERNET_SPI_SCK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
	};
	int spi_dma_channel = 2;
#ifdef MODDEF_ETHERNET_SPI_DMA_CH
	spi_dma_channel = MODDEF_ETHERNET_SPI_DMA_CH;
#endif
	ESP_ERROR_CHECK(spi_bus_initialize(MODDEF_ETHERNET_SPI_PORT, &buscfg, spi_dma_channel));
#else
	modSPIConfigurationRecord config = {0};
	config.cs_pin = MODDEF_ETHERNET_SPI_CS_PIN;
	config.spiPort = HSPI_HOST;
	config.sync = 1;
	config.hz = MODDEF_ETHERNET_HZ;
	config.command_bits = MODDEF_ETHERNET_SPI_COMMAND_BITS;
	config.address_bits = MODDEF_ETHERNET_SPI_ADDRESS_BITS;
	config.external = 1;
	config.queue_size = 20;
	modSPIInit(&config);
#endif
}
#endif

static void initEthernet(void)
{
	if (gNetif) return;

	esp_netif_init();
	esp_err_t err = esp_event_loop_create_default();
	if (ESP_ERR_INVALID_STATE != err)
		ESP_ERROR_CHECK(err);

	esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
	gNetif = esp_netif_new(&netif_cfg);

	esp_eth_mac_t *mac;
	esp_eth_phy_t *phy;
	esp_eth_handle_t eth_handle;

#ifdef MODDEF_ETHERNET_POWER_PIN
	gpio_set_direction(MODDEF_ETHERNET_POWER_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(MODDEF_ETHERNET_POWER_PIN, 0);
	vTaskDelay(pdMS_TO_TICKS(100));
	gpio_set_level(MODDEF_ETHERNET_POWER_PIN, 1);
	vTaskDelay(pdMS_TO_TICKS(200));
#endif

#ifdef MODDEF_ETHERNET_INTERNAL_PHY_GPIO0_CLOCK_INPUT
	gpio_reset_pin(GPIO_NUM_0);
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_FLOATING);
	vTaskDelay(pdMS_TO_TICKS(50));
#endif

#ifdef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
	gpio_set_pull_mode(MODDEF_ETHERNET_INTERNAL_MAC_MDIO, GPIO_PULLUP_ONLY);

	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	eth_esp32_emac_config_t esp32_mac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
	esp32_mac_config.smi_gpio.mdc_num = MODDEF_ETHERNET_INTERNAL_MAC_MDC;
	esp32_mac_config.smi_gpio.mdio_num = MODDEF_ETHERNET_INTERNAL_MAC_MDIO;
	mac = esp_eth_mac_new_esp32(&esp32_mac_config, &mac_config);

	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.phy_addr = MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS;
	phy_config.reset_gpio_num = MODDEF_ETHERNET_INTERNAL_PHY_RESET;
	phy = esp_eth_phy_new_generic(&phy_config);
#else
	err = gpio_install_isr_service(0);
	if (ESP_OK != err && ESP_ERR_INVALID_STATE != err)
		ESP_ERROR_CHECK(err);

	spi_device_interface_config_t devcfg = {
		.command_bits = MODDEF_ETHERNET_SPI_COMMAND_BITS,
		.address_bits = MODDEF_ETHERNET_SPI_ADDRESS_BITS,
		.mode = 0,
		.clock_speed_hz = MODDEF_ETHERNET_HZ,
		.spics_io_num = MODDEF_ETHERNET_SPI_CS_PIN,
		.queue_size = 20
	};

	init_spi();
	mac = mod_ethernet_get_mac(devcfg, MODDEF_ETHERNET_INT_PIN);
	phy = mod_ethernet_get_phy();
#endif

	esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
	err = esp_eth_driver_install(&eth_config, &eth_handle);
	if (ESP_OK != err) {
		mac->del(mac);
		phy->del(phy);
		esp_netif_destroy(gNetif);
		gNetif = C_NULL;
		return;
	}

	uint8_t macaddr[6];
	ESP_ERROR_CHECK(esp_read_mac(macaddr, ESP_MAC_ETH));
	mac->set_addr(mac, macaddr);

	ESP_ERROR_CHECK(esp_netif_attach(gNetif, esp_eth_new_netif_glue(eth_handle)));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, doEthernetEvent, C_NULL, C_NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, doIPEvent, C_NULL, C_NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_GOT_IP6, doIPEvent, C_NULL, C_NULL));

	ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

static void doEthernetEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	xsEthernet walker;
	EthernetEventMsg msg;
	msg.ipInit = 0;

	if (ETHERNET_EVENT_CONNECTED == event_id) {
		gConnected = 1;
		gIP = 0;
		if (ESP_OK != esp_netif_create_ip6_linklocal(gNetif))
			gIP = 0x02;
		msg.ipInit = gIP;
	}
	else if (ETHERNET_EVENT_DISCONNECTED == event_id) {
		gConnected = 0;
		gIP = 0;
	}
	else
		return;

	msg.event_id = event_id;
	for (walker = gEthernetList; walker; walker = walker->next) {
		__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
		modMessagePostToMachine(walker->the, (uint8_t *)&msg, sizeof(msg), ethernetConnectDeliver, walker);
	}
}

static void doIPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	xsEthernet walker;

	if (IP_EVENT_GOT_IP6 == event_id) {
		if (0x03 == gIP) return;
		gIP |= 0x02;
		if (0x03 != gIP) return;
		event_id = IP_EVENT_ETH_GOT_IP;
	}
	else if (IP_EVENT_ETH_GOT_IP == event_id) {
		gIP |= 0x01;
		if (0x03 != gIP) return;
	}
	else
		return;

	EthernetEventMsg msg;
	msg.event_id = event_id;
	msg.ipInit = 0;
	for (walker = gEthernetList; walker; walker = walker->next) {
		__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
		modMessagePostToMachine(walker->the, (uint8_t *)&msg, sizeof(msg), ethernetConnectDeliver, walker);
	}
}

static void ethernetConnectDeliver(void *the, void *refcon, uint8_t *msgIn, uint16_t msgLen)
{
	xsEthernet eth = refcon;
	EthernetEventMsg *msg = (EthernetEventMsg *)msgIn;
	uint8_t prevConnected = eth->connected, prevConnecting = eth->connecting, prevIP = eth->ip;

	if (eth->closed)
		goto bail;

	if (ETHERNET_EVENT_CONNECTED == msg->event_id) {
		eth->connected = 1;
		eth->ip = msg->ipInit;
	}
	else if (ETHERNET_EVENT_DISCONNECTED == msg->event_id) {
		eth->connected = 0;
		eth->connecting = 0;
		eth->ip = 0;
	}
	else if (IP_EVENT_ETH_GOT_IP == msg->event_id) {
		eth->connected = 1;
		eth->ip = 0x03;
	}

	if (eth->onChanged &&
		((prevConnected != eth->connected) ||
		 (prevConnecting != eth->connecting) ||
		 (prevIP != eth->ip))) {
		xsBeginHost(the);
		xsCallFunction0(xsReference(eth->onChanged), eth->obj);
		xsEndHost(the);
	}

bail:
	xs_ethernet_destructor(eth);
}

void xs_ethernet_connect(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	if (eth->connecting)
		xsUnknownError("already connecting");

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_static)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_static);

		esp_netif_ip_info_t ip_info = {0};

		xsSlot tmp;
		xsmcGet(tmp, xsVar(0), xsID_address);
		esp_netif_str_to_ip4(xsmcToString(tmp), &ip_info.ip);

		xsmcGet(tmp, xsVar(0), xsID_mask);
		esp_netif_str_to_ip4(xsmcToString(tmp), &ip_info.netmask);

		xsmcGet(tmp, xsVar(0), xsID_gateway);
		esp_netif_str_to_ip4(xsmcToString(tmp), &ip_info.gw);

		esp_netif_dhcpc_stop(gNetif);
		if (ESP_OK != esp_netif_set_ip_info(gNetif, &ip_info))
			xsUnknownError("set IP failed");

		gIP = 0x03;

		EthernetEventMsg msg;
		msg.event_id = IP_EVENT_ETH_GOT_IP;
		msg.ipInit = 0;
		__atomic_add_fetch(&eth->useCount, 1, __ATOMIC_SEQ_CST);
		modMessagePostToMachine(eth->the, (uint8_t *)&msg, sizeof(msg), ethernetConnectDeliver, eth);
	}
	else {
		eth->connecting = 1;
		esp_netif_dhcpc_start(gNetif);
	}
}

void xs_ethernet_disconnect(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	eth->connecting = 0;
	eth->ip = 0;
}

void xs_ethernet_connection_get(xsMachine *the)
{
	xsEthernet eth = xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);
	int connection;

	if (eth->connected && (0x03 == eth->ip))
		connection = 500;
	else if (eth->connecting)
		connection = 300;
	else if (eth->connected)
		connection = 400;
	else
		connection = 200;

	xsmcSetInteger(xsResult, connection);
}

void xs_ethernet_address_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);

	if (!gNetif) return;

	esp_netif_ip_info_t info;
	if (ESP_OK == esp_netif_get_ip_info(gNetif, &info) && info.ip.addr) {
		xsResult = xsStringBuffer(NULL, 39);
		esp_ip4addr_ntoa(&info.ip, xsmcToString(xsResult), 40);
		return;
	}

	esp_ip6_addr_t ip6;
	if (ESP_OK == esp_netif_get_ip6_linklocal(gNetif, &ip6)) {
		xsResult = xsStringBuffer(NULL, 39);
		ip6addr_ntoa_r((ip6_addr_t *)&ip6, xsmcToString(xsResult), 40);
	}
}

void xs_ethernet_MAC_get(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsEthernetHooks);
	uint8_t mac[6];

	if (ESP_OK != esp_read_mac(mac, ESP_MAC_ETH))
		return;

	xsResult = xsStringBuffer(NULL, 17);
	formatMAC(mac, xsmcToString(xsResult));
}
