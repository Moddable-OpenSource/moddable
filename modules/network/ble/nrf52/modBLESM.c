/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
#include "xsHost.h"
#include "mc.xs.h"
#include "modBLE.h"
#include "modBLECommon.h"
#include "peer_manager.h"

#define SEC_PARAM_MIN_KEY_SIZE 7
#define SEC_PARAM_MAX_KEY_SIZE 16

static void pm_evt_handler(pm_evt_t const * p_evt);

void xs_ble_sm_delete_all_bondings(xsMachine *the)
{
	pm_peers_delete();
}

void xs_ble_sm_delete_bonding(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
	
	ble_gap_addr_t peer_addr;
	c_memmove(peer_addr.addr, address, 6);
	peer_addr.addr_type = addressType;
	modBLEBondingRemove(the, &peer_addr);
}

uint16_t modBLESetSecurityParameters(uint8_t encryption, uint8_t bonding, uint8_t mitm, uint16_t ioCapability)
{
	uint8_t io_caps;
	ble_gap_sec_params_t sec_param;
	ret_code_t err_code;

	c_memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));
	
 	switch(ioCapability) {
 		case NoInputNoOutput:
 			io_caps = BLE_GAP_IO_CAPS_NONE;
 			break;
 		case DisplayOnly:
 			io_caps = BLE_GAP_IO_CAPS_DISPLAY_ONLY;
 			break;
 		case KeyboardOnly:
 			io_caps = BLE_GAP_IO_CAPS_KEYBOARD_ONLY;
 			break;
 		case KeyboardDisplay:
 			io_caps = BLE_GAP_IO_CAPS_KEYBOARD_DISPLAY;
 			break;
 		case DisplayYesNo:
 			io_caps = BLE_GAP_IO_CAPS_DISPLAY_YESNO;
 			break;
 	} 	

	if (BLE_GAP_IO_CAPS_NONE == io_caps)
		mitm = 0;
		
    sec_param.bond           = bonding;
    sec_param.mitm           = mitm;
    sec_param.lesc           = (mitm && (io_caps == BLE_GAP_IO_CAPS_DISPLAY_YESNO));
    sec_param.io_caps        = io_caps;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    if (bonding) {
		sec_param.kdist_own.enc  = 1;
		sec_param.kdist_own.id   = 1;
		sec_param.kdist_peer.enc = 1;
		sec_param.kdist_peer.id  = 1;
    }

	err_code = pm_sec_params_set(&sec_param);
	return err_code;
}
