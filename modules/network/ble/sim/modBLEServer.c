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

void xs_ble_server_initialize(xsMachine *the)
{
}

void xs_ble_server_close(xsMachine *the)
{
}

void xs_ble_server_destructor(void *data)
{
}

void xs_ble_server_get_local_address(xsMachine *the)
{
	const uint8_t addr[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	xsmcSetArrayBuffer(xsResult, (void*)addr, 6);
}

void xs_ble_server_set_device_name(xsMachine *the)
{
}

void xs_ble_server_start_advertising(xsMachine *the)
{
}
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
}

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
}

void xs_ble_server_deploy(xsMachine *the)
{
}

void xs_ble_server_get_service_attributes(xsMachine *the)
{
}

void xs_ble_server_disconnect(xsMachine *the)
{
}

void xs_ble_server_passkey_input(xsMachine *the)
{
}

void xs_ble_server_passkey_reply(xsMachine *the)
{
}

void xs_ble_server_set_security_parameters(xsMachine *the)
{
}
