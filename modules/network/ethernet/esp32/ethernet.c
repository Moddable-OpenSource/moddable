/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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

#include "xs.h"
#include "xsmc.h"
#include "xsHost.h"

#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "esp_eth.h"
#include "esp_netif.h"

#include "ethernet.h"
#include "esp_eth_phy.h"

#include "modSPI.h"


int8_t gEthernetState = -2;	// -2 = uninitialized, -1 = gpio isr initialized, 0 = not started, 1 = starting, 2 = started, 3 = connecting, 4 = connected, 5 = IP address
int8_t	gEthernetIP = 0;		// 0x01 == IP4, 0x02 == IP6

esp_netif_t *gNetif = NULL;

static esp_err_t initEthernet(void);

#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
static spi_device_handle_t init_spi();
static void uninit_spi(spi_device_handle_t);
#endif

void xs_ethernet_start(xsMachine *the)
{
	esp_err_t err = initEthernet();
	if (err != ESP_OK)
		xsUnknownError("Ethernet device not found");
}

typedef struct xsEthernetRecord xsEthernetRecord;
typedef xsEthernetRecord *xsEthernet;

struct xsEthernetRecord {
	xsEthernet				next;
	xsMachine				*the;
	xsSlot					obj;
	uint8_t					haveCallback;
};

static xsEthernet gEthernet;

static void ethernetEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
    xsEthernet ethernet = refcon;
    int32_t event_id = *(int32_t *)message;
    const char *msg;


    switch (event_id) {
        case ETHERNET_EVENT_START:          msg = "start"; break;
        case ETHERNET_EVENT_CONNECTED:      msg = "connect"; break;
        case ETHERNET_EVENT_DISCONNECTED:   msg = "disconnect"; break;
        default: return;
    }

    xsBeginHost(the);
		xsCall1(ethernet->obj, xsID_callback, xsString(msg));
    xsEndHost(the);
}

static void ipEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
    xsEthernet ethernet = refcon;
    int32_t event_id = *(int32_t *)message;
    const char *msg;

    switch (event_id) {
        case IP_EVENT_ETH_GOT_IP:            msg = "gotIP"; break;
        default: return;
    }

    xsBeginHost(the);
        xsCall1(ethernet->obj, xsID_callback, xsString(msg));
    xsEndHost(the);
}

void xs_ethernet_destructor(void *data)
{
	xsEthernet ethernet = data;

	if (ethernet) {
		if (ethernet == gEthernet)
			gEthernet = ethernet->next;
		else {
			xsEthernet walker;
			for (walker = gEthernet; walker->next != ethernet; walker = walker->next)
				;
			walker->next = ethernet->next;
		}

		c_free(ethernet);
	}
}

void xs_ethernet_constructor(xsMachine *the)
{
	int argc = xsmcArgc;

	if (1 == argc)
		xsCall1(xsThis, xsID_build, xsArg(0));
}

void xs_ethernet_close(xsMachine *the)
{
	xsEthernet ethernet = xsmcGetHostData(xsThis);
	if (ethernet) {
		if (ethernet->haveCallback)
			xsForget(ethernet->obj);
	}
	xs_ethernet_destructor(ethernet);
	xsmcSetHostData(xsThis, NULL);
}

void xs_ethernet_set_onNotify(xsMachine *the)
{
	xsEthernet ethernet = xsmcGetHostData(xsThis);

	if (NULL == ethernet) {
		ethernet = c_calloc(1, sizeof(xsEthernetRecord));
		if (!ethernet)
			xsUnknownError("out of memory");
		xsmcSetHostData(xsThis, ethernet);
		ethernet->the = the;
		ethernet->obj = xsThis;
	}
	else if (ethernet->haveCallback) {
		xsmcDelete(xsThis, xsID_callback);
		ethernet->haveCallback = false;
		xsForget(ethernet->obj);
	}

	if (!xsmcTest(xsArg(0)))
		return;

	ethernet->haveCallback = true;

	xsRemember(ethernet->obj);

	ethernet->next = gEthernet;
	gEthernet = ethernet;

	xsmcSet(xsThis, xsID_callback, xsArg(0));
}

static void doEthernetEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	xsEthernet walker;

	esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

	switch (event_id) {
		case ETHERNET_EVENT_START:
#ifdef MODDEF_ETHERNET_HOSTNAME
			esp_netif_set_hostname(gNetif, MODDEF_ETHERNET_HOSTNAME);
#endif
			gEthernetState = 3;
			break;
		case ETHERNET_EVENT_STOP:
			gEthernetState = 1;
			break;
		case ETHERNET_EVENT_CONNECTED:
			gEthernetState = 4;
			gEthernetIP = 0;

            if (ESP_OK != esp_netif_create_ip6_linklocal(gNetif))
				gEthernetIP = 0x02;		// don't wait for IP6 address if esp_netif_create_ip6_linklocal failed
			break;
		case ETHERNET_EVENT_DISCONNECTED:
			gEthernetState = 2;
			gEthernetIP = 0;
			break;
		default:
            return;
	}

	for (walker = gEthernet; NULL != walker; walker = walker->next)
		modMessagePostToMachine(walker->the, (uint8_t *)&event_id, sizeof(event_id), ethernetEventPending, walker);
}

static void doIPEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    xsEthernet walker;
    switch (event_id) {
        case IP_EVENT_GOT_IP6:
            if (0x03 == gEthernetIP)
                return;
            gEthernetIP |= 0x02;
            if (0x03 != gEthernetIP)
                return;
            event_id = IP_EVENT_ETH_GOT_IP;
            gEthernetState = 5;
            break;

        case IP_EVENT_ETH_GOT_IP:
            gEthernetIP |= 0x01;
            gEthernetState = 5;
            break;
    }

    for (walker = gEthernet; NULL != walker; walker = walker->next)
        modMessagePostToMachine(walker->the, (uint8_t *)&event_id, sizeof(event_id), ipEventPending, walker);
}

esp_err_t initEthernet(void)
{
	spi_device_handle_t spi_handle;
	uint8_t macaddr[6];
	esp_err_t err = ESP_OK;
	esp_eth_mac_t *mac;
	esp_eth_phy_t *phy;
	esp_eth_handle_t eth_handle;

	if (gEthernetState > 0) return err;

	esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
	gNetif = esp_netif_new(&netif_cfg);

#ifdef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
	eth_mac_config_t mac_config =  ETH_MAC_DEFAULT_CONFIG();
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

	phy_config.phy_addr = MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS;
	phy_config.reset_gpio_num = MODDEF_ETHERNET_INTERNAL_PHY_RESET;
	mac_config.smi_mdc_gpio_num = MODDEF_ETHERNET_INTERNAL_MAC_MDC;
	mac_config.smi_mdio_gpio_num = MODDEF_ETHERNET_INTERNAL_MAC_MDIO;
	mac = esp_eth_mac_new_esp32(&mac_config);
#if MODDEF_ETHERNET_INTERNAL_PHY_IP101
    phy = esp_eth_phy_new_ip101(&phy_config);
#elif MODDEF_ETHERNET_INTERNAL_PHY_RTL8201
    phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif MODDEF_ETHERNET_INTERNAL_PHY_LAN87XX
    phy = esp_eth_phy_new_lan87xx(&phy_config);
#elif MODDEF_ETHERNET_INTERNAL_PHY_DP83848
    phy = esp_eth_phy_new_dp83848(&phy_config);
#elif MODDEF_ETHERNET_INTERNAL_PHY_KSZ8041
    phy = esp_eth_phy_new_ksz8041(&phy_config);
#elif MODDEF_ETHERNET_INTERNAL_PHY_KSZ8081
    phy = esp_eth_phy_new_ksz8081(&phy_config);
#endif
#else
	if (gEthernetState < -1) {
		gpio_install_isr_service(0);
		gEthernetState = -1;
	}
		
	spi_handle = init_spi();
	mac = mod_ethernet_get_mac(spi_handle, MODDEF_ETHERNET_INT_PIN);
	phy = mod_ethernet_get_phy();
#endif
	
	esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
	eth_handle = NULL;
	err = esp_eth_driver_install(&eth_config, &eth_handle);
	if (err != ESP_OK) {
		if (mac)
			mac->del(mac);
		if (phy)
			phy->del(phy);
#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
		if (spi_handle)
			uninit_spi(spi_handle);
#endif
		return err;
	}

	ESP_ERROR_CHECK(esp_netif_init());
	err = esp_event_loop_create_default();
	if (ESP_ERR_INVALID_STATE != err)		// ESP_ERR_INVALID_STATE indicates the default event loop has already been created
		ESP_ERROR_CHECK(err);

	gEthernetState = 1;

	ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &doEthernetEvent, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &doIPEvent, NULL, NULL));

	ESP_ERROR_CHECK(esp_read_mac(macaddr, ESP_MAC_ETH));
	mac->set_addr(mac, macaddr);

	ESP_ERROR_CHECK(esp_netif_attach(gNetif, esp_eth_new_netif_glue(eth_handle)));
	ESP_ERROR_CHECK(esp_eth_start(eth_handle));

	return err;
}


#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
static spi_device_handle_t init_spi() {
	spi_device_handle_t spi_handle = NULL;

#ifdef MODDEF_ETHERNET_SPI_MISO_PIN
	spi_bus_config_t buscfg = {
		.miso_io_num = MODDEF_ETHERNET_SPI_MISO_PIN,
        .mosi_io_num = MODDEF_ETHERNET_SPI_MOSI_PIN,
        .sclk_io_num = MODDEF_ETHERNET_SPI_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
	};
	ESP_ERROR_CHECK(spi_bus_initialize(MODDEF_ETHERNET_SPI_PORT, &buscfg, 2));
	spi_device_interface_config_t devcfg = {
        .command_bits = 3,
        .address_bits = 5,
        .mode = 0,
        .clock_speed_hz = MODDEF_ETHERNET_HZ,
        .spics_io_num =  MODDEF_ETHERNET_SPI_CS_PIN,
        .queue_size = 20
    };
    ESP_ERROR_CHECK(spi_bus_add_device(MODDEF_ETHERNET_SPI_PORT, &devcfg, &spi_handle));
#else
	modSPIConfigurationRecord config = {0};
	config.cs_pin = MODDEF_ETHERNET_SPI_CS_PIN;
	config.spiPort = HSPI_HOST;
	config.sync = 1;
	config.hz = MODDEF_ETHERNET_HZ;
	config.command_bits = 3; 	// requires a related change to pins/spi
	config.address_bits = 5;
	config.external = 1;
	config.queue_size = 20;
	modSPIInit(&config);
	spi_handle = config.spi_dev;
#endif

	return spi_handle;
}

static void uninit_spi(spi_device_handle_t spi_handle) {
#ifdef MODDEF_ETHERNET_SPI_MISO_PIN
	spi_bus_remove_device(spi_handle);
    spi_bus_free(MODDEF_ETHERNET_SPI_PORT);
#else
	modSPIConfigurationRecord config = {0};
	config.cs_pin = MODDEF_ETHERNET_SPI_CS_PIN;
	config.spiPort = HSPI_HOST;
	config.sync = 1;
	config.hz = MODDEF_ETHERNET_HZ;
	config.command_bits = 3; 	// requires a related change to pins/spi
	config.address_bits = 5;
	config.external = 1;
	config.queue_size = 20;
	modSPIUninit(&config);
#endif
}
#endif
