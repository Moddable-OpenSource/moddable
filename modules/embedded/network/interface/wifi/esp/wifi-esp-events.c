/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

#include "wifi-esp-events.h"
#include "xsHost.h"

#define WIFI_ESP_MAX_LISTENERS 4

static WiFiEventListener gListeners[WIFI_ESP_MAX_LISTENERS];
static uint8_t gInitialized;

static void sharedHandler(System_Event_t *msg)
{
	for (int i = 0; i < WIFI_ESP_MAX_LISTENERS; i++) {
		if (gListeners[i])
			gListeners[i](msg);
	}
}

void wifi_esp_events_addListener(WiFiEventListener cb)
{
	for (int i = 0; i < WIFI_ESP_MAX_LISTENERS; i++) {
		if (gListeners[i] == cb)
			return;
	}
	for (int i = 0; i < WIFI_ESP_MAX_LISTENERS; i++) {
		if (!gListeners[i]) {
			gListeners[i] = cb;
			if (!gInitialized) {
				wifi_set_event_handler_cb(sharedHandler);
				gInitialized = 1;
			}
			return;
		}
	}
}

void wifi_esp_events_removeListener(WiFiEventListener cb)
{
	for (int i = 0; i < WIFI_ESP_MAX_LISTENERS; i++) {
		if (gListeners[i] == cb) {
			gListeners[i] = C_NULL;
			return;
		}
	}
}
