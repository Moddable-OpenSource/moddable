/*
 * Copyright (c) 2016-2025 Moddable Tech, Inc.
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
#include "mc.defines.h"		// Moddable defines

// Define LOG_LOCAL_LEVEL before including esp_log.h to enable compile-time logging
#ifndef LOG_LOCAL_LEVEL
	#define LOG_LOCAL_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#endif
#include "esp_log.h"
#include "esp_eth.h"
#include "esp_eth_driver.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "ethernet.h"

#include "modSPI.h"

#include "driver/gpio.h"		// Need to control the GPIO pins for the Ethernet board

#include "lwip/ip_addr.h"		// Need to set the IP address for the Ethernet board	


static const char *TAG = "mod.ethernet.c";
/* Formats a log string to prepend context function name */
#define LOG_FMT(x) "%s: " x, __func__

// If these are not defined in the manifest.json, use the default values
#ifndef MODDEF_ETHERNET_SPI_COMMAND_BITS
   #define MODDEF_ETHERNET_SPI_COMMAND_BITS (3)
#endif
#ifndef MODDEF_ETHERNET_SPI_ADDRESS_BITS
   #define MODDEF_ETHERNET_SPI_ADDRESS_BITS (5)
#endif
#ifndef MODDEF_ETHERNET_HZ
   #define MODDEF_ETHERNET_HZ (16000000)
#endif
#ifndef MODDEF_ETHERNET_DEBUG	
   #define MODDEF_ETHERNET_DEBUG (0)
#endif

int8_t gEthernetState = -2;	// -2 = uninitialized, -1 = gpio isr initialized, 0 = not started, 1 = starting, 2 = started, 3 = connecting, 4 = connected, 5 = IP address
int8_t	gEthernetIP = 0;		// 0x01 == IP4, 0x02 == IP6

esp_netif_t *gNetif = NULL;

static esp_err_t initEthernet(xsMachine *the);

// For SPI Ethernet boards, the MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS is blank and we need to use the SPI interface to talk to the Ethernet board
#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
static void init_spi();
static void uninit_spi();
#endif

void xs_ethernet_start(xsMachine *the)
{
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Starting Ethernet"));
	esp_err_t err = initEthernet(the);
	if (err != ESP_OK)
		xsUnknownError("Ethernet.c Ethernet device not found");
}

void xs_ethernet_useStaticIP(xsMachine *the)
{
	if (!gNetif)
		xsUnknownError("Ethernet not initialized");
	
	// Parse and set IP configuration
	esp_netif_ip_info_t ip_info;
	esp_err_t err;
	
	char *ipAddr = xsmcToString(xsArg(0));
	err = esp_netif_str_to_ip4(ipAddr, &ip_info.ip);
	if (err != ESP_OK)
		xsUnknownError("Invalid IP address format");
	
	char *netmask = xsmcToString(xsArg(1));
	err = esp_netif_str_to_ip4(netmask, &ip_info.netmask);
	if (err != ESP_OK)
		xsUnknownError("Invalid netmask format");
	
	char *gateway = xsmcToString(xsArg(2));
	err = esp_netif_str_to_ip4(gateway, &ip_info.gw);
	if (err != ESP_OK)
		xsUnknownError("Invalid gateway format");
	
	// Stop DHCP client first
	esp_netif_dhcpc_stop(gNetif);

	err = esp_netif_set_ip_info(gNetif, &ip_info);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, LOG_FMT("Failed to set static IP: %d"), err);
		xsUnknownError("Failed to set static IP");
	}
	
	// Read back and verify the actual IP configuration
	esp_netif_ip_info_t actual_ip_info;
	err = esp_netif_get_ip_info(gNetif, &actual_ip_info);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, LOG_FMT("Static IP configured - IP: " IPSTR ", Netmask: " IPSTR ", Gateway: " IPSTR),
			IP2STR(&actual_ip_info.ip), IP2STR(&actual_ip_info.netmask), IP2STR(&actual_ip_info.gw));
	} else {
		ESP_LOGE(TAG, LOG_FMT("Failed to read back IP info: %d. Static IP configuration failed"), err);
		xsUnknownError("Failed to set static IP");
	}
	xsmcSetInteger(xsResult, 0);  // Return success if we got here
}	// xs_ethernet_setStaticIP

/*
Switches the Ethernet interface to DHCP mode
*/
void xs_ethernet_useDHCP(xsMachine *the)
{
	if (!gNetif)
		xsUnknownError("Ethernet not initialized");
	
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Switching to DHCP mode"));
	
	// Clear any static IP
	esp_netif_ip_info_t ip_info;
	memset(&ip_info, 0, sizeof(ip_info));
	esp_netif_set_ip_info(gNetif, &ip_info);
	
	// Start DHCP client
	esp_err_t err = esp_netif_dhcpc_start(gNetif);
	if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
		ESP_LOGE(TAG, LOG_FMT("Failed to start DHCP client: %d"), err);
		xsUnknownError("Failed to start DHCP client");
	}
	
	ESP_LOGI(TAG, LOG_FMT("DHCP client started successfully"));
	xsmcSetInteger(xsResult, 0);  // Return success
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
			xsUnknownError("Ethernet.c Out of memory");
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
			ESP_LOGI(TAG, LOG_FMT("Ethernet event: ETHERNET_EVENT_START"));
#ifdef MODDEF_ETHERNET_HOSTNAME
			esp_netif_set_hostname(gNetif, MODDEF_ETHERNET_HOSTNAME);
			if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Set hostname: %s"), MODDEF_ETHERNET_HOSTNAME);	
#endif
			gEthernetState = 3;
			break;
		case ETHERNET_EVENT_STOP:
			ESP_LOGI(TAG, LOG_FMT("Ethernet event: ETHERNET_EVENT_STOP"));
			gEthernetState = 1;
			break;
		case ETHERNET_EVENT_CONNECTED:
			{
				ESP_LOGI(TAG, LOG_FMT("Ethernet event: ETHERNET_EVENT_CONNECTED"));
			gEthernetState = 4;
			gEthernetIP = 0;

				// Get link speed and duplex
				eth_speed_t speed;
				eth_duplex_t duplex;
				esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
				esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
				esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex);
				ESP_LOGI(TAG, LOG_FMT("Link up: %s, %s"), 
					speed == ETH_SPEED_100M ? "100Mbps" : "10Mbps",
					duplex == ETH_DUPLEX_FULL ? "Full-Duplex" : "Half-Duplex");

				// Check if netif is up
				if (esp_netif_is_netif_up(gNetif)) {
					ESP_LOGI(TAG, LOG_FMT("Netif is UP"));
				} else {
					ESP_LOGW(TAG, LOG_FMT("Netif is DOWN! Glue layer failed to start netif"));
					return;
				}

				esp_err_t ipv6_result = esp_netif_create_ip6_linklocal(gNetif);
				if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("IPv6 link-local creation result: %d"), ipv6_result);
				if (ESP_OK != ipv6_result) {
					ESP_LOGW(TAG, LOG_FMT("IPv6 link-local creation failed: %d, skipping IPv6"), ipv6_result);
				gEthernetIP = 0x02;		// don't wait for IP6 address if esp_netif_create_ip6_linklocal failed
				} else {
					if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("IPv6 link-local creation initiated"));
				}
			}
			break;
		case ETHERNET_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, LOG_FMT("Ethernet event: ETHERNET_EVENT_DISCONNECTED"));
			gEthernetState = 2;
			gEthernetIP = 0;
			break;
		default:
			ESP_LOGW(TAG, LOG_FMT("Unknown Ethernet event: %d"), event_id);
            return;
	}

	// Send ethernetEventPending event to all JavaScript listeners
	for (walker = gEthernet; NULL != walker; walker = walker->next)
		modMessagePostToMachine(walker->the, (uint8_t *)&event_id, sizeof(event_id), ethernetEventPending, walker);
}

static void doIPEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    xsEthernet walker;
    if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("IP Event=%d, current gEthernetIP=0x%02X"), event_id, gEthernetIP);
    
    switch (event_id) {
        case IP_EVENT_GOT_IP6:
            {
                ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
                ESP_LOGI(TAG, LOG_FMT("Got IPv6 address: " IPV6STR), IPV62STR(event->ip6_info.ip));
            if (0x03 == gEthernetIP)
                return;
            gEthernetIP |= 0x02;
                if (0x03 != gEthernetIP) {
                    if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Waiting for IPv4 (gEthernetIP=0x%02X)"), gEthernetIP);
                return;
                }
            event_id = IP_EVENT_ETH_GOT_IP;
            gEthernetState = 5;
                if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Both IPv4 and IPv6 ready, notifying application"));
            }
            break;

        case IP_EVENT_ETH_GOT_IP:
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, LOG_FMT("Got IPv4 address: " IPSTR " Gateway: " IPSTR " Netmask: " IPSTR), 
				IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.gw), IP2STR(&event->ip_info.netmask));
            gEthernetIP |= 0x01;
            gEthernetState = 5;
                if ((gEthernetIP & 0x02) == 0) {
                    if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Waiting for IPv6 (gEthernetIP=0x%02X)"), gEthernetIP);
                } else {
                    if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Both IPv4 and IPv6 ready, notifying application"));
                }
            }
            break;
    }

	// Send ipEventPending event to all JavaScript listeners
    for (walker = gEthernet; NULL != walker; walker = walker->next)
        modMessagePostToMachine(walker->the, (uint8_t *)&event_id, sizeof(event_id), ipEventPending, walker);
}

esp_err_t initEthernet(xsMachine *the)
{
	uint8_t macaddr[6];
	esp_err_t err = ESP_OK;
	esp_eth_mac_t *mac;
	esp_eth_phy_t *phy;
	esp_eth_handle_t eth_handle;

	ESP_LOGI(TAG, LOG_FMT("Initializing Ethernet"));

	if (gEthernetState > 0) return err;

	// Initialize netif and event loop first (required before creating netif objects)
	ESP_ERROR_CHECK(esp_netif_init());
	err = esp_event_loop_create_default();
	if (ESP_ERR_INVALID_STATE != err)		// ESP_ERR_INVALID_STATE indicates the default event loop has already been created
		ESP_ERROR_CHECK(err);

	esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
	gNetif = esp_netif_new(&netif_cfg);

#ifdef MODDEF_ETHERNET_POWER_PIN
	// Turn ethernet phy on. This is needed if you control power to the Ethernet board.
	// Perform a hard reset by power cycling
	ESP_LOGI(TAG, LOG_FMT("Applying Power using GPIO Pin=%d"), MODDEF_ETHERNET_POWER_PIN);
    gpio_set_direction(MODDEF_ETHERNET_POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MODDEF_ETHERNET_POWER_PIN, 0);  // First Power off for 100ms to ensure we turn it back on
	vTaskDelay(pdMS_TO_TICKS(100));    // Hold in reset/off
    gpio_set_level(MODDEF_ETHERNET_POWER_PIN, 1);  // Power on
	vTaskDelay(pdMS_TO_TICKS(200));    // LAN8720A needs ~200ms to stabilize after power-on
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Power applied to Ethernet"));
#endif

#ifdef MODDEF_ETHERNET_INTERNAL_PHY_GPIO0_CLOCK_INPUT
	// Configure GPIO0 as input to prevent conflict with external 50MHz oscillator (RMII_CLK)
	// This is critical for WT32-ETH01 and similar boards with external clock source
	gpio_reset_pin(GPIO_NUM_0);
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_FLOATING);
	vTaskDelay(pdMS_TO_TICKS(50));    // Allow clock to stabilize
	ESP_LOGI(TAG, LOG_FMT("Configured GPIO0 as input for external RMII clock"));
#endif	// #ifdef MODDEF_ETHERNET_INTERNAL_PHY_GPIO0_CLOCK_INPUT
#ifdef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
	// Ensure MDIO has pull-up (open-drain bidirectional bus)
	gpio_set_pull_mode(MODDEF_ETHERNET_INTERNAL_MAC_MDIO, GPIO_PULLUP_ONLY);
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Configured MDIO (GPIO%d) with pull-up"), MODDEF_ETHERNET_INTERNAL_MAC_MDIO);

	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	eth_esp32_emac_config_t esp32_mac_config =  ETH_ESP32_EMAC_DEFAULT_CONFIG();
	esp32_mac_config.smi_gpio.mdc_num = MODDEF_ETHERNET_INTERNAL_MAC_MDC;
	esp32_mac_config.smi_gpio.mdio_num = MODDEF_ETHERNET_INTERNAL_MAC_MDIO;
	// Explicit clock configuration (uncomment if sdkconfig settings aren't working)
	// esp32_mac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
	// esp32_mac_config.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;
	// ESP_LOGI(TAG, LOG_FMT("Initialize Ethernet ESP32 MAC with MDC=%d, MDIO=%d, external clock on GPIO0"), 
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Initialize Ethernet ESP32 MAC with MDC=%d and MDIO=%d"), 
		MODDEF_ETHERNET_INTERNAL_MAC_MDC, MODDEF_ETHERNET_INTERNAL_MAC_MDIO);
	mac = esp_eth_mac_new_esp32(&esp32_mac_config, &mac_config);

#if 0
	// Scan for PHY at all addresses to find where it's responding (diagnostic)
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Scanning for PHY on MDIO bus..."));
	uint32_t phy_id_reg;
	int found_addr = -1;
	for (int addr = 0; addr < 32; addr++) {
		if (mac->read_phy_reg(mac, addr, 0x02, &phy_id_reg) == ESP_OK) {
			if (phy_id_reg != 0x0000 && phy_id_reg != 0xFFFF) {
				ESP_LOGI(TAG, LOG_FMT("PHY found at address %d, ID1=0x%04X"), addr, (unsigned int)phy_id_reg);
				found_addr = addr;
			}
		}
	}
	if (found_addr < 0) {
		ESP_LOGE(TAG, LOG_FMT("No PHY found on MDIO bus! Check wiring and clock."));
		xsUnknownError("Ethernet.c No PHY found on MDIO bus! Check wiring and clock.\n");		//@@
	}
	if (found_addr != MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS) {
		ESP_LOGW(TAG, LOG_FMT("PHY found at address %d but configured for %d"), found_addr, MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS);
	}
#endif
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.phy_addr = MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS;
	phy_config.reset_gpio_num = MODDEF_ETHERNET_INTERNAL_PHY_RESET;
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
	// SPI Ethernet device
	if (gEthernetState < -1) {
		gpio_install_isr_service(0);
		gEthernetState = -1;
	}
	ESP_LOGI(TAG, LOG_FMT("Initialize Ethernet SPI"));
	spi_device_interface_config_t devcfg = {
		.command_bits = MODDEF_ETHERNET_SPI_COMMAND_BITS,
		.address_bits = MODDEF_ETHERNET_SPI_ADDRESS_BITS,
        .mode = 0,
        .clock_speed_hz = MODDEF_ETHERNET_HZ,
        .spics_io_num =  MODDEF_ETHERNET_SPI_CS_PIN,
        .queue_size = 20
    };

	init_spi();
	mac = mod_ethernet_get_mac(devcfg, MODDEF_ETHERNET_INT_PIN);
	phy = mod_ethernet_get_phy();
#endif	// #ifdef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
	
	esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
	eth_handle = NULL;
	ESP_LOGI(TAG, LOG_FMT("Installing Ethernet driver"));
	err = esp_eth_driver_install(&eth_config, &eth_handle);
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Installing Ethernet driver result=%d"), err);
	if (err != ESP_OK) {
		if (mac)
			mac->del(mac);
		if (phy)
			phy->del(phy);
#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
		uninit_spi();
#endif
		return err;
	}

	gEthernetState = 1;

	ESP_ERROR_CHECK(esp_read_mac(macaddr, ESP_MAC_ETH));
	mac->set_addr(mac, macaddr);
	ESP_LOGI(TAG, LOG_FMT("MAC address: %02X:%02X:%02X:%02X:%02X:%02X"), 
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

	// Attach Network interface to Ethernet driver BEFORE registering event handlers (glue needs to register its handlers first)
	ESP_ERROR_CHECK(esp_netif_attach(gNetif, esp_eth_new_netif_glue(eth_handle)));
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Netif attached to Ethernet driver"));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &doEthernetEvent, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &doIPEvent, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_GOT_IP6, &doIPEvent, NULL, NULL));
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Event handlers registered for Ethernet and IP events"));

	// Start DHCP client by default
	esp_netif_dhcp_status_t dhcp_status;
	esp_netif_dhcpc_get_status(gNetif, &dhcp_status);
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("DHCP client status: %d (0=init, 1=started, 2=stopped)"), dhcp_status);

	if (dhcp_status == ESP_NETIF_DHCP_INIT) {
		if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Starting DHCP client..."));
		err = esp_netif_dhcpc_start(gNetif);
		if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
			ESP_LOGE(TAG, LOG_FMT("Failed to start DHCP client: %d"), err);
			ESP_LOGW(TAG, LOG_FMT("DHCP may not work - consider using setStaticIP()"));
		} else {
			ESP_LOGI(TAG, LOG_FMT("DHCP client started successfully"));
		}
	}
	
	ESP_ERROR_CHECK(esp_eth_start(eth_handle));
	ESP_LOGI(TAG, LOG_FMT("Ethernet started, waiting for link and DHCP..."));

	return err;
}

#ifndef MODDEF_ETHERNET_INTERNAL_PHY_ADDRESS
/*
	Initializes the SPI interface for the Ethernet board
*/
static void init_spi() {
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
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("MISO=%d MOSI=%d SCK=%d CS=%d DMA_Ch=%d PORT=%d HZ=%d PowerPin=%d"), 
		MODDEF_ETHERNET_SPI_MISO_PIN, MODDEF_ETHERNET_SPI_MOSI_PIN, MODDEF_ETHERNET_SPI_SCK_PIN, 
		spi_dma_channel,MODDEF_ETHERNET_SPI_CS_PIN, MODDEF_ETHERNET_SPI_PORT, MODDEF_ETHERNET_HZ, MODDEF_ETHERNET_POWER_PIN);

	ESP_ERROR_CHECK(spi_bus_initialize(MODDEF_ETHERNET_SPI_PORT, &buscfg, MODDEF_ETHERNET_SPI_DMA_CH ));
#else
	modSPIConfigurationRecord config = {0};
	config.cs_pin = MODDEF_ETHERNET_SPI_CS_PIN;
	config.spiPort = HSPI_HOST;
	config.sync = 1;
	config.hz = MODDEF_ETHERNET_HZ;
	config.command_bits = MODDEF_ETHERNET_SPI_COMMAND_BITS,
	config.address_bits = MODDEF_ETHERNET_SPI_ADDRESS_BITS,
	config.external = 1;
	config.queue_size = 20;
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("MISO=%d MOSI=%d SCK=%d CS=%d DMA_Ch=%d PORT=%d HZ=%d PowerPin=%d"), 
		config.miso_pin, config.mosi_pin, config.clock_pin, config.cs_pin, spi_dma_channel, 
		config.spiPort, config.hz, MODDEF_ETHERNET_POWER_PIN);
	modSPIInit(&config);
#endif
}

static void uninit_spi() {
	if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, LOG_FMT("Uninitializing SPI"));
#ifdef MODDEF_ETHERNET_SPI_MISO_PIN
    spi_bus_free(MODDEF_ETHERNET_SPI_PORT);
#else
	modSPIConfigurationRecord config = {0};
	config.cs_pin = MODDEF_ETHERNET_SPI_CS_PIN;
	config.spiPort = HSPI_HOST;
	config.sync = 1;
	config.hz = MODDEF_ETHERNET_HZ;
#ifdef MODDEF_ETHERNET_SPI_COMMAND_BITS
	config.command_bits = MODDEF_ETHERNET_SPI_COMMAND_BITS,
#else
	config.command_bits = 3,		// requires a related change to pins/spi
#endif
	config.address_bits = MODDEF_ETHERNET_SPI_ADDRESS_BITS,
	config.external = 1;
	config.queue_size = 20;
	modSPIUninit(&config);
#endif
}
#endif
