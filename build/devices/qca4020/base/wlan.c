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

#define QAPI_USE_WLAN
#include "qapi.h"

#ifndef WLAN_NUM_OF_DEVICES
    #define WLAN_NUM_OF_DEVICES 1	// maximum of 2 allowed
#endif

static uint8_t gWLANEnabled = 0;
static uint32_t gActiveDevice = 0;

int8_t qca4020_wlan_enable(void)
{
	uint8_t i, enabled = (gWLANEnabled == 1);
	
	if (enabled)
		return 0;
		
	if (0 == qapi_WLAN_Enable(QAPI_WLAN_ENABLE_E)) {
		for (i = 0; i < WLAN_NUM_OF_DEVICES; i++)
			qapi_WLAN_Add_Device(i);
		gWLANEnabled = 1;
		return 0;
	}
	
    return -1;
}

int8_t qca4020_wlan_disable(void)
{
	uint8_t i, enabled = (gWLANEnabled == 1);
	
	if (!enabled)
		return 0;
		
	for (i = 0; i < WLAN_NUM_OF_DEVICES; i++)
		qapi_WLAN_Remove_Device(i);
		
	if (0 == qapi_WLAN_Enable(QAPI_WLAN_DISABLE_E))
		return 0;
		
	return -1;
}

void qca4020_wlan_set_active_device(uint8_t deviceId)
{
	if (deviceId < WLAN_NUM_OF_DEVICES) {
		qca4020_wlan_enable();
		gActiveDevice = deviceId;
	}
}


int8_t qca4020_wlan_get_active_device(void)
{
	qca4020_wlan_enable();
	return gActiveDevice;
}

