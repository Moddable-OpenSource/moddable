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

#include "xsmc.h"
#include "xsesp.h"
#include "mc.xs.h"
#include "modBLE.h"
#include "host/ble_store.h"
#include "host/ble_hs.h"

void xs_ble_sm_delete_all_bondings(xsMachine *the)
{
	int i, rc, count;
	ble_addr_t *peer_addrs = NULL;
	rc = ble_store_util_count(BLE_STORE_OBJ_TYPE_OUR_SEC, &count);
	if (0 == rc && count > 0) {
		peer_addrs = c_malloc(count * sizeof(ble_addr_t));
		if (NULL != peer_addrs) {
			rc = ble_store_util_bonded_peers(peer_addrs, &count, count);
			if (0 == rc) {
				for (i = 0; i < count; ++i)
					ble_store_util_delete_peer(&peer_addrs[i]);
			}
		}
	}
	if (NULL != peer_addrs)
		c_free(peer_addrs);
}

void modBLESetSecurityParameters(uint8_t encryption, uint8_t bonding, uint8_t mitm, uint16_t ioCapability)
{
  	switch(ioCapability) {
 		case NoInputNoOutput:
 			ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
 			break;
 		case DisplayOnly:
 			ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
 			break;
 		case KeyboardOnly:
 			ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_KEYBOARD_ONLY;
 			break;
 		case KeyboardDisplay:
 			ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_KEYBOARD_DISP;
 			break;
 		case DisplayYesNo:
 			ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_YES_NO;
 			break;
 	} 	
 	if (bonding) {
		ble_hs_cfg.sm_bonding = 1;
		ble_hs_cfg.sm_our_key_dist = 1;
    	ble_hs_cfg.sm_their_key_dist = 1;
 	}
    ble_hs_cfg.sm_mitm = mitm ? 1 : 0;
    
	// https://github.com/espressif/esp-idf/issues/3532
    // Enabling LE 4.2 secure connections in the app leads to pairing requests to be ignored
	// The ble_gap_security_initiate() function called from the server does not
	// trigger the pairing request.
	//ble_hs_cfg.sm_sc = 1;
}

