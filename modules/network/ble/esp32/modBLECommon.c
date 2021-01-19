/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#include "esp_bt.h"
#include "esp_bt_main.h"

static int16_t useCount = 0;

int modBLEPlatformInitialize(void)
{
	if (0 != useCount++)
		return 0;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_err_t err = esp_bt_controller_init(&bt_cfg);
	if (ESP_OK == err)
		err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ESP_OK == err)
		err = esp_bluedroid_init();
	if (ESP_OK == err)
		err = esp_bluedroid_enable();
	return err;
}

int modBLEPlatformTerminate(void)
{
	if (0 != --useCount)
		return 0;

	modBLEWhitelistClear();
	
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();
	
	return 0;
}
