/*
 * Wiznet W5500 Driver for Moddable SDK
 *
 * For this to work, you must ensure:
 * 1) Edit the manifest.json to ensure the SPI pin connections are correct in the ethernet defines.
 * 		"ethernet": {
 *		    "enc28j60": 0,
 *        	"w5500": 1,
 *          "hz": 16000000,
 *          "int_pin": 14,
 *			"hostname":"\"Moddable\"",
 *            "spi": {
 *				"command_bits": 16,
 *				"address_bits": 8,
 *              "cs_pin": 10,
 *              "port": "SPI2_HOST",
 *              "miso_pin": 13,
 *              "mosi_pin": 11,
 *              "sck_pin": 12,
 *				"polling_ms": 0,
 *				"dma_ch": 3
 *          }
 *      }
 * 2) The sdkconfig.default file has these lines (see build\devices\esp32\targets\esp32s3_cdc\sdkconfig\sdkconfig.defaults)
 *      #
 *      # Ethernet
 *      #
 *      CONFIG_ETH_ENABLED=y
 *      CONFIG_ETH_USE_SPI_ETHERNET=y
 *      CONFIG_ETH_SPI_ETHERNET_W5500=y
 *      # end of Ethernet
 * 3) In %MODDABLE%\build\devices\esp32\xsProj-esp32s3\main\CMakeLists.txt, the 
 * add_prebuilt_library needs to include esp_driver_spi esp_eth esp_netif log
 *      add_prebuilt_library(xsesp32 ${CMAKE_BINARY_DIR}/xs_${ESP32_SUBCLASS}.a
 *			REQUIRES esp_timer esp_wifi spi_flash bt nvs_flash spiffs fatfs esp_lcd esp_driver_gpio esp_driver_spi esp_eth esp_netif log  ${ESP_COMPONENTS}
 *		)
 * Other ESP32 devices would need to modify their CMakeLists.txt
 *
 * Copyright (c) 2021 Moddable Tech, Inc.
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

#include "mc.defines.h" // Moddable defines
#if MODDEF_ETHERNET_W5500
#include "esp_eth_phy.h"
#include "esp_eth_mac.h"
#include "esp_eth_com.h"
#include "esp_eth_mac_spi.h"
#include "esp_log.h"

#ifndef MODDEF_ETHERNET_DEBUG
	#define MODDEF_ETHERNET_DEBUG (0)
#endif

static const char *TAG = "mod.w5500_api.c";

esp_eth_mac_t* mod_ethernet_get_mac(spi_device_interface_config_t spi_devcfg, int interrupt_pin)
{
    if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, "mod_ethernet_get_mac");
	/* ESP code w5500 ethernet driver is based on spi driver
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(CONFIG_EXAMPLE_ETH_SPI_HOST, &spi_devcfg);
    w5500_config.int_gpio_num = CONFIG_EXAMPLE_ETH_SPI_INT_GPIO;
	w5500_config.int_gpio_num = spi_eth_module_config->int_gpio;
    w5500_config.poll_period_ms = spi_eth_module_config->polling_ms;
    s_mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    s_phy = esp_eth_phy_new_w5500(&phy_config);*/
	eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(MODDEF_ETHERNET_SPI_PORT, &spi_devcfg);
    w5500_config.int_gpio_num = interrupt_pin;
	w5500_config.poll_period_ms = MODDEF_ETHERNET_SPI_POLLING_MS;
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.rx_task_prio = 2;
	return esp_eth_mac_new_w5500(&w5500_config, &mac_config);
}

esp_eth_phy_t* mod_ethernet_get_phy()
{
    if (MODDEF_ETHERNET_DEBUG) ESP_LOGI(TAG, "mod_ethernet_get_phy");
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0;
    phy_config.reset_gpio_num = -1;
	return esp_eth_phy_new_w5500(&phy_config);
}

#endif
