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

#include "xsmc.h"
#include "xsgecko.h"
#include "mc.xs.h"
#include "modBLE.h"

#include "bg_types.h"
#include "native_gecko.h"

void xs_ble_sm_delete_all_bondings(xsMachine *the)
{
	gecko_cmd_sm_delete_bondings();
}

void xs_ble_sm_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint16_t ioCapability = xsmcToInteger(xsArg(3));
	uint8_t flags = 0;
	uint8_t io_capabilities;
 	switch(ioCapability) {
 		case NoInputNoOutput:
 			io_capabilities = sm_io_capability_noinputnooutput;
 			break;
 		case DisplayOnly:
 			io_capabilities = sm_io_capability_displayonly;
 			break;
 		case KeyboardOnly:
 			io_capabilities = sm_io_capability_keyboardonly;
 			break;
 		case KeyboardDisplay:
 			io_capabilities = sm_io_capability_keyboarddisplay;
 			break;
 		case DisplayYesNo:
 			io_capabilities = sm_io_capability_displayyesno;
 			break;
 	} 	
 	
	if (mitm)
		flags |= 0x1;
		
	gecko_cmd_sm_configure(flags, io_capabilities);
	
	setSecurityParameters(encryption, bonding, mitm);
}

