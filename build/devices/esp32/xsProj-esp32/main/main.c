/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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


#define __XS6PLATFORMMINIMAL__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_task_wdt.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_log.h"

#if CONFIG_BT_ENABLED
	#include "esp_bt.h"
#endif

#include "modInstrumentation.h"
#include "esp_system.h"		// to get system_get_free_heap_size, etc.

#include "xs.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "mc.defines.h"

#if MODDEF_ECMA419_ENABLED
	#include "common/builtinCommon.h"
#endif

extern void mc_setup(xsMachine *the);
extern void	setupDebugger();

#if 0 == CONFIG_LOG_DEFAULT_LEVEL
	#define kStack (((8 * 1024) + XT_STACK_EXTRA_CLIB) / sizeof(StackType_t))
#else
	#define kStack (((10 * 1024) + XT_STACK_EXTRA_CLIB) / sizeof(StackType_t))
#endif

#if MODDEF_SOFTRESET
	uint8_t gSoftReset;
#endif

xsMachine *gThe;		// the main XS virtual machine running

#ifdef mxDebug
	// #define WEAK __attribute__((weak))
	#define WEAK

	/*
		xsbug IP address

		IP address either:
			0,0,0,0 - no xsbug connection
			127,0,0,7 - xsbug over serial
			w,x,y,z - xsbug over TCP (address of computer running xsbug)
	*/

	#define XSDEBUG_NONE 0,0,0,0
	#define XSDEBUG_SERIAL 127,0,0,7
	#ifndef DEBUG_IP
		#define DEBUG_IP XSDEBUG_SERIAL
	#endif

	WEAK unsigned char gXSBUG[4] = {DEBUG_IP};
#endif

void loop_task(void *pvParameter)
{
#if CONFIG_ESP_TASK_WDT_EN
	esp_task_wdt_add(NULL);
#endif

	while (true) {
#if MODDEF_SOFTRESET
		gSoftReset = 0;
#endif

		gThe = modCloneMachine(NULL, NULL);

		modRunMachineSetup(gThe);

#if MODDEF_SOFTRESET
		xsMachine *the = gThe;
		while (!gSoftReset) {
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}

		xsDeleteMachine(the);
#else
		while (true) {
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
#endif
	}
}

void app_main() {
	modPrelaunch();

#if defined(CONFIG_LOG_DEFAULT_LEVEL) && (CONFIG_LOG_DEFAULT_LEVEL > 0)
	esp_log_level_set("wifi", ESP_LOG_ERROR);
	esp_log_level_set("phy_init", ESP_LOG_ERROR);
	esp_log_level_set("I2S", ESP_LOG_ERROR);
#endif

	ESP_ERROR_CHECK(nvs_flash_init());
#if CONFIG_BT_ENABLED
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
#endif

	setupDebugger();

	xTaskCreate(loop_task, "main", kStack, NULL, 4, NULL);
}

