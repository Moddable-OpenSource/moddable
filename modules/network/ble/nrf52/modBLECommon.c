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

#include "xsHost.h"
#include "modBLECommon.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"

const uint16_t primary_service_uuid = 0x2800;
const uint16_t character_declaration_uuid = 0x2803;
const uint16_t character_extended_properties_uuid = 0x2900;
const uint16_t character_user_descriptor_uuid = 0x2901;
const uint16_t character_client_config_uuid = 0x2902;
const uint16_t character_presentation_format_uuid = 0x2904;

static int16_t useCount = 0;

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
	uint32_t ram_start = 0;
	
	if (0 != useCount++)
		return NRF_SUCCESS;

	// Initialize platform Bluetooth modules
    err_code = nrf_sdh_enable_request();
    
	// Configure the BLE stack using the default BLE settings defined in the sdk_config.h file.
	// Fetch the start address of the application RAM.
    if (NRF_SUCCESS == err_code)
		err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);

    // Enable BLE stack
    if (NRF_SUCCESS == err_code)
    	err_code = nrf_sdh_ble_enable(&ram_start);

	// Initialize GATT module
    if (NRF_SUCCESS == err_code)
		err_code = nrf_ble_gatt_init(init->p_gatt, NULL);
    
	// Initialize connection parameters (required)
    if (NRF_SUCCESS == err_code)
		err_code = ble_conn_params_init(&init->cp_init);

	// Initialize the peer manager
    if (NRF_SUCCESS == err_code)
		err_code = pm_init();

	// Register peer manager event handler
	if (NRF_SUCCESS == err_code)
		err_code = pm_register(init->pm_event_handler);

	// Create a FreeRTOS task for the BLE stack.
    if (NRF_SUCCESS == err_code)
		nrf_sdh_freertos_init(NULL, NULL);
    
	return err_code;
}

ret_code_t modBLEPlatformTerminate(void)
{
	if (0 != --useCount)
		return NRF_SUCCESS;

	return nrf_sdh_disable_request();
}
