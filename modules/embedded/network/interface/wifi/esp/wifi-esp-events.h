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

// Shared Wi-Fi event dispatcher for ESP8266. The NONOS SDK accepts only one
// wifi_set_event_handler_cb; this shim owns that slot and fans events out to
// listeners registered by wifi-esp.c (station) and wifiaccesspoint-esp.c (AP).

#pragma once

#include "user_interface.h"

typedef void (*WiFiEventListener)(System_Event_t *msg);

void wifi_esp_events_addListener(WiFiEventListener cb);
void wifi_esp_events_removeListener(WiFiEventListener cb);
