/*
 * Copyright (c) 2020  Moddable Tech, Inc.
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

/*
	WakeableAnalog

	To do:
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_* values

#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_gpio.h"

#define kPinReset (128)

void xs_wakeableanalog_constructor(xsMachine *the)
{
}

void xs_wakeableanalog_destructor(void *data)
{
}

void xs_wakeableanalog_close(xsMachine *the)
{
}

void xs_wakeableanalog_read(xsMachine *the)
{
}

void wakeableAnalogDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

