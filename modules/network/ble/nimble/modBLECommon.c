/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

#include "xsHost.h"
#include "modBLECommon.h"
#include "mc.defines.h"

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "esp_bt.h"
#include "esp_nimble_hci.h"

static int16_t useCount = 0;

static void nimble_on_reset(int reason)
{
	// fatal controller reset - all connections have been closed
#if MODDEF_BLE_CLIENT
	ble_client_on_reset();
#endif
#if MODDEF_BLE_SERVER
	ble_server_on_reset();
#endif
}

static void nimble_host_task(void *param)
{
	nimble_port_run();
	nimble_port_freertos_deinit();
}

static void ble_host_task(void *param)
{
	nimble_host_task(param);
}

static esp_err_t _esp_nimble_hci_and_controller_init(void)
{
    esp_err_t err;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	err = esp_bt_controller_init(&bt_cfg);
	if (ESP_OK == err)
		err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ESP_OK == err)
		err = esp_nimble_hci_init();

    return err;
}

int modBLEPlatformInitialize(void)
{
	if (0 != useCount++) {
		ble_hs_cfg.sync_cb();
		return 0;
	}

	ble_hs_cfg.reset_cb = nimble_on_reset;

	esp_err_t err = _esp_nimble_hci_and_controller_init();
	
	if (ESP_OK == err) {
		nimble_port_init();
	
		ble_store_config_init();
		
		nimble_port_freertos_init(ble_host_task);
	}

	return err;
}

int modBLEPlatformTerminate(void)
{
	if (0 != --useCount)
		return 0;
		
	modBLEWhitelistClear();
	
	int rc = nimble_port_stop();
	if (0 == rc) {
		nimble_port_deinit();
		esp_nimble_hci_and_controller_deinit();
	}
	
	return rc;
}
