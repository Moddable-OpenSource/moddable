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

#ifndef __mod_ble_common__
#define __mod_ble_common__

#include "xsmc.h"
#include "ble.h"
#include "ble_conn_params.h"
#include "nrf_ble_gatt.h"
#include "peer_manager_types.h"

#define APP_BLE_CONN_CFG_TAG 1

#define GATT_CHAR_PROP_BIT_READ		(1 << 1)
#define GATT_CHAR_PROP_BIT_WRITE_NR	(1 << 2)
#define GATT_CHAR_PROP_BIT_WRITE	(1 << 3)
#define GATT_CHAR_PROP_BIT_NOTIFY	(1 << 4)
#define GATT_CHAR_PROP_BIT_INDICATE	(1 << 5)
#define GATT_CHAR_PROP_BIT_AUTH		(1 << 6)
#define GATT_CHAR_PROP_BIT_EXT_PROP	(1 << 7)

#define GATT_PERM_READ				(1 << 0)
#define GATT_PERM_READ_ENCRYPTED	(1 << 1)
#define GATT_PERM_WRITE				(1 << 4)
#define GATT_PERM_WRITE_ENCRYPTED	(1 << 5)

#define CHAR_DECLARATION_SIZE		(sizeof(uint8_t))
#define UUID_LEN_16		2
#define UUID_LEN_32		4
#define UUID_LEN_128	16

typedef struct {
	uint16_t uuid_length;
	uint8_t  *uuid_p;
	uint16_t perm;
	uint16_t max_length;
	uint16_t length;
	uint8_t  *value;
} attr_desc_t;

typedef struct {
	attr_desc_t att_desc;
} gatts_attr_db_t;

typedef struct {
	uint8_t *uuid;
} vendor_specific_uuid_t;

typedef struct {
	nrf_ble_gatt_t *p_gatt;
	ble_conn_params_init_t cp_init;
	pm_evt_handler_t pm_event_handler;
	uint16_t vs_uuid_count;
	const vendor_specific_uuid_t *p_vs_uuids;
} modBLEPlatformInitializeDataRecord, *modBLEPlatformInitializeData;

extern const uint16_t primary_service_uuid;
extern const uint16_t character_declaration_uuid;
extern const uint16_t character_client_config_uuid;
extern const uint16_t character_extended_properties_uuid;
extern const uint16_t character_user_descriptor_uuid;
extern const uint16_t character_presentation_format_uuid;

ret_code_t modBLEPlatformInitialize(modBLEPlatformInitializeData init);
ret_code_t modBLEPlatformTerminate(void);

void uuidToBuffer(uint8_t *buffer, ble_uuid_t *uuid, uint16_t *length);
void bufferToUUID(ble_uuid_t *uuid, uint8_t *buffer, uint16_t length);

void modBLEBondingRemove(xsMachine *the, ble_gap_addr_t *peer_addr);
void modBLEClientBondingRemoved(ble_gap_addr_t *peer_addr);
void modBLEServerBondingRemoved(ble_gap_addr_t *peer_addr);

#endif