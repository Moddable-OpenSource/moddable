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
#include "mc.defines.h"
#include "mc.xs.h"
#include "modBLE.h"
#include "host/ble_store.h"
#include "host/ble_hs.h"

static void deleteBonding(xsMachine *the, uint8_t *address, uint8_t addressType);

void xs_ble_sm_delete_all_bondings(xsMachine *the)
{
	deleteBonding(the, NULL, 0);
}

void xs_ble_sm_delete_bonding(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
	
	deleteBonding(the, address, addressType);
}

void deleteBonding(xsMachine *the, uint8_t *address, uint8_t addressType)
{
	ble_addr_t *peer_addrs = NULL;
	int rc, i, count;
	
	rc = ble_store_util_count(BLE_STORE_OBJ_TYPE_OUR_SEC, &count);
	if (0 == rc && count > 0) {
		peer_addrs = c_malloc(count * sizeof(ble_addr_t));
		if (!peer_addrs)
			xsUnknownError("no memory");
		if (0 == ble_store_util_bonded_peers(peer_addrs, &count, count)) {
			for (i = 0; i < count; ++i) {
				ble_addr_t *addr = &peer_addrs[i];
				if (NULL == address || (addressType == addr->type && 0 == c_memcmp(address, addr->val, 6))) {
					ble_store_util_delete_peer(addr);
#if MODDEF_BLE_CLIENT
					modBLEClientBondingRemoved(address, addressType);
#endif
#if MODDEF_BLE_SERVER
					modBLEServerBondingRemoved(address, addressType);
#endif
					break;
				}
			}
		}
	}
	if (NULL != peer_addrs)
		c_free(peer_addrs);
}

uint16_t modBLESetSecurityParameters(uint8_t encryption, uint8_t bonding, uint8_t mitm, uint16_t ioCapability)
{
	ble_hs_cfg.sm_sc = 1;	// always enable secure connections
	ble_hs_cfg.sm_mitm = mitm ? 1 : 0;
	ble_hs_cfg.sm_bonding = bonding ? 1 : 0;

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
		ble_hs_cfg.sm_our_key_dist = 1 | BLE_SM_PAIR_KEY_DIST_ID;
		ble_hs_cfg.sm_their_key_dist = 1 | BLE_SM_PAIR_KEY_DIST_ID;
	}
		
	return 0;
}

