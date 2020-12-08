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
#include "xsHost.h"
#include "mc.defines.h"
#include "modBLECommon.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "sdk_config.h"

typedef struct modBLEBondRecord modBLEBondRecord;
typedef modBLEBondRecord *modBLEBond;

struct modBLEBondRecord {
	struct modBLEBondRecord *next;

	xsMachine *the;
	pm_peer_id_t peer_id;
	ble_gap_addr_t peer_addr;
};

const uint16_t primary_service_uuid = 0x2800;
const uint16_t character_declaration_uuid = 0x2803;
const uint16_t character_extended_properties_uuid = 0x2900;
const uint16_t character_user_descriptor_uuid = 0x2901;
const uint16_t character_client_config_uuid = 0x2902;
const uint16_t character_presentation_format_uuid = 0x2904;

static void peer_delete_pm_evt_handler(pm_evt_t const * p_evt);

static int16_t useCount = 0;
static modBLEBond gBonds = NULL;

void uuidToBuffer(uint8_t *buffer, ble_uuid_t *uuid, uint16_t *length)
{
	uint8_t uuid_le_len;
	sd_ble_uuid_encode(uuid, &uuid_le_len, buffer);
	*length = uuid_le_len;
}

void bufferToUUID(ble_uuid_t *uuid, uint8_t *buffer, uint16_t length)
{
	sd_ble_uuid_decode(length, buffer, uuid);
}

ret_code_t modBLEPlatformInitialize(modBLEPlatformInitializeData init)
{
	ret_code_t err_code;
    ble_cfg_t ble_cfg;
	uint32_t ram_start = 0;
	static uint8_t pm_initialized = false;
	
	if (0 != useCount++)
		return NRF_SUCCESS;

	// Initialize platform Bluetooth modules
    err_code = nrf_sdh_enable_request();

	// Configure the BLE stack using the default BLE settings defined in the sdk_config.h file.
	// Fetch the start address of the application RAM.
    if (NRF_SUCCESS == err_code)
		err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);

	// Override specific configuration options from the initialization data
	if (NRF_SUCCESS == err_code) {
		c_memset(&ble_cfg, 0, sizeof(ble_cfg));
		ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = init->vs_uuid_count;
		err_code = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_cfg, ram_start);
	}
	
    // Enable BLE stack
    if (NRF_SUCCESS == err_code)
    	err_code = nrf_sdh_ble_enable(&ram_start);

	// Initialize GATT module
    if (NRF_SUCCESS == err_code)
		err_code = nrf_ble_gatt_init(init->p_gatt, NULL);
    
    // @@ We disable the connection parameters module by default, since otherwise the Nordic SDK is unable to successfully reinitialize the BLE stack.
#if NRF_BLE_CONN_PARAMS_ENABLED
	modLog("warning: unable to reinitialize ble when connection parameters module is enabled");
	modLog("https://devzone.nordicsemi.com/f/nordic-q-a/57834/reenable-ble-after-disabling-fails-in-ble_conn_params_init-on-freertos");
	// Initialize connection parameters
	if (NRF_SUCCESS == err_code)
		err_code = ble_conn_params_init(&init->cp_init);
#endif

	// Add vendor-specific 128-bit uuids
	for (int i = 0; NRF_SUCCESS == err_code && i < init->vs_uuid_count; ++i) {
		uint8_t uuid_type;
		ble_uuid128_t ble_uuid_128 = *(ble_uuid128_t*)init->p_vs_uuids[i].uuid;
		err_code = sd_ble_uuid_vs_add(&ble_uuid_128, &uuid_type);
	}

	// Initialize the peer manager - this can only happen once because there is no API to uninitialize the peer manager
    if (!pm_initialized && (NRF_SUCCESS == err_code)) {
		err_code = pm_init();
		if (NRF_SUCCESS == err_code)
			pm_initialized = 1;

		// Register peer manager event handlers
		if (NRF_SUCCESS == err_code)
			err_code = pm_register(init->pm_event_handler);
		if (NRF_SUCCESS == err_code)
			err_code =  pm_register(peer_delete_pm_evt_handler);
    }

	// Create a FreeRTOS task for the BLE stack.
    if (NRF_SUCCESS == err_code)
		nrf_sdh_freertos_init(NULL, NULL);
    
	return err_code;
}

ret_code_t modBLEPlatformTerminate(void)
{
	if (0 != --useCount)
		return NRF_SUCCESS;

#if NRF_BLE_CONN_PARAMS_ENABLED
	ble_conn_params_stop();
#endif

	modBLEBond walker = gBonds;
	while (NULL != walker) {
		modBLEBond next = walker->next;
		c_free(walker);
		walker = next;
	}

	return nrf_sdh_disable_request();
}

void modBLEBondingRemove(xsMachine *the, ble_gap_addr_t *peer_addr)
{
    pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    while (peer_id != PM_PEER_ID_INVALID) {
		ret_code_t err_code;
        pm_peer_data_bonding_t p_data;
        err_code = pm_peer_data_bonding_load(peer_id, &p_data);
        if (NRF_SUCCESS == err_code) {
			ble_gap_addr_t *p_bonded_peer_addr = &(p_data.peer_ble_id.id_addr_info);
			uint8_t match;

			// first check for a public address match
			match = (peer_addr->addr_type == p_bonded_peer_addr->addr_type && 0 == c_memcmp(peer_addr->addr, p_bonded_peer_addr->addr, 6));
			
			// next check for a private random resolvable address match
			if (!match)
				match = pm_address_resolve(peer_addr, &(p_data.peer_ble_id.id_info));
			
			if (match) {
				modBLEBond bond;
				bond = c_malloc(sizeof(modBLEBondRecord));
				if (NULL != bond) {
					bond->peer_id = peer_id;
					bond->peer_addr = *peer_addr;
					bond->the = the;
					bond->next = NULL;
				}
				if (!gBonds)
					gBonds = bond;
				else {
					modBLEBond walker;
					for (walker = gBonds; walker->next; walker = walker->next)
						;
					walker->next = bond;
				}
				pm_peer_delete(peer_id);
				return;
			}
        }
		peer_id = pm_next_peer_id_get(peer_id);
    }
}

static void pmPeerDeleteSucceededEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ble_gap_addr_t *peer_addr = (ble_gap_addr_t *)message;
	
#if MODDEF_BLE_CLIENT
	modBLEClientBondingRemoved(peer_addr);
#endif
#if MODDEF_BLE_SERVER
	modBLEServerBondingRemoved(peer_addr);
#endif
}

void peer_delete_pm_evt_handler(pm_evt_t const * p_evt)
{
    switch (p_evt->evt_id) {
		case PM_EVT_PEER_DELETE_SUCCEEDED: {
			modBLEBond walker, prev = NULL;
			for (walker = gBonds; NULL != walker; prev = walker, walker = walker->next) {
				if (p_evt->peer_id == walker->peer_id) {
					if (NULL == prev)
						gBonds = walker->next;
					else
						prev->next = walker->next;
					modMessagePostToMachine(walker->the, (uint8_t*)&walker->peer_addr, sizeof(ble_gap_addr_t), pmPeerDeleteSucceededEvent, NULL);
					c_free(walker);
					break;
				}
			}
			break;
		}
        default:
            break;
    }
}
