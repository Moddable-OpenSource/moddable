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
#include "xsHost.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "modBLE.h"
#include "modBLECommon.h"

#include "sdk_errors.h"
#include "sdk_config.h"
#include "ble.h"
#include "ble_conn_params.h"
#include "ble_gap.h"
#include "ble_radio_notification.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "nrf_ble_gatt.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"

#define APP_BLE_OBSERVER_PRIO 3
#define FIRST_CONN_PARAMS_UPDATE_DELAY      5000                                    /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       30000                                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define LOG_GATTS 0
#if LOG_GATTS
	#define LOG_GATTS_EVENT(event) logGATTSEvent(event)
	#define LOG_GATTS_MSG(msg) modLog(msg)
	#define LOG_GATTS_INT(i) modLogInt(i)
#else
	#define LOG_GATTS_EVENT(event)
	#define LOG_GATTS_MSG(msg)
	#define LOG_GATTS_INT(i)
#endif

#define LOG_GAP 0
#if LOG_GAP
	#define LOG_GAP_EVENT(event) logGAPEvent(event)
	#define LOG_GAP_MSG(msg) modLog(msg)
	#define LOG_GAP_INT(i) modLogInt(i)
#else
	#define LOG_GAP_EVENT(event)
	#define LOG_GAP_MSG(msg)
	#define LOG_GAP_INT(i)
#endif

#define LOG_PM 0
#if LOG_PM
	#define LOG_PM_EVENT(event) logPMEvent(event)
	#define LOG_PM_MSG(msg) modLog(msg)
	#define LOG_PM_INT(i) modLogInt(i)
#else
	#define LOG_PM_EVENT(event)
	#define LOG_PM_MSG(msg)
	#define LOG_PM_INT(i)
#endif

#define DEVICE_FRIENDLY_NAME "Moddable"

#include "mc.bleservices.c"

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void pm_evt_handler(pm_evt_t const * p_evt);
static void ble_radio_evt(bool active);

static void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bleServerReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void gapAuthKeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapPasskeyDisplayEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void gattsReadAuthRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattsWriteAuthRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattsWriteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void bleRadioOffEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void ble_radio_notification_deinit();

static void setAttributePermissions(security_req_t *read_access, security_req_t *write_access, security_req_t *cccd_write_access, uint8_t permissions);
static void setAttributeProperties(ble_gatt_char_props_t *char_props, uint16_t properties);

static const char_name_table *handleToCharName(uint16_t handle);

typedef struct {
	xsMachine *the;
	xsSlot obj;

	uint8_t deviceNameSet;

	// connection
	uint16_t conn_handle;
	ble_gap_addr_t remote_bda;
	
	// gatt
	nrf_ble_gatt_t m_gatt;
	
	// advertising
	uint8_t adv_handle;
	uint8_t adv_notify;
	uint8_t adv_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
	uint8_t scan_rsp_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
	
	// services
	uint8_t deployServices;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;
} modBLERecord, *modBLE;

static modBLE gBLE = NULL;

#if MODDEF_BLE_MAX_CONNECTIONS != 1
	#error - only one ble client connection supported
#endif

void xs_ble_server_initialize(xsMachine *the)
{
    ret_code_t err_code;
	modBLEPlatformInitializeDataRecord init;
	ble_conn_params_init_t *p_cp_init;
    
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->conn_handle = BLE_CONN_HANDLE_INVALID;
	gBLE->adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
	gBLE->iocap = 0xFF;
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsmcSetHostData(xsThis, gBLE);
	xsRemember(gBLE->obj);
	
	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_deployServices)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_deployServices);
		gBLE->deployServices = xsmcToBoolean(xsVar(0));
	}
	else
		gBLE->deployServices = true;

	// Initialize platform Bluetooth modules
	init.p_gatt = &gBLE->m_gatt;
	init.pm_event_handler = pm_evt_handler;
	init.vs_uuid_count = vendor_specific_uuid_count;
	init.p_vs_uuids = vendor_uuids;
	p_cp_init = &init.cp_init;
	c_memset(p_cp_init, 0, sizeof(ble_conn_params_init_t));
	p_cp_init->first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	p_cp_init->next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	p_cp_init->max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	
	err_code = modBLEPlatformInitialize(&init);
	
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble initialization failed");

    // Register a handler for GAP and GATTC events
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

	// Notify app that stack is ready
	modMessagePostToMachine(the, NULL, 0, bleServerReadyEvent, NULL);
}

void xs_ble_server_close(xsMachine *the)
{
	modBLE ble = xsmcGetHostData(xsThis);
	if (!ble) return;

	gBLE = NULL;
	xsForget(ble->obj);
	xsmcSetHostData(xsThis, NULL);
	modMessagePostToMachine(ble->the, NULL, 0, bleServerCloseEvent, ble);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;

	if (ble->adv_notify) {
		sd_ble_gap_adv_stop(ble->adv_handle);
		ble_radio_notification_deinit();
	}
	
	if (-1 != ble->conn_handle)
		sd_ble_gap_disconnect(ble->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	
	c_free(ble);
	gBLE = NULL;

	modBLEPlatformTerminate();
}

void xs_ble_server_disconnect(xsMachine *the)
{
	if (BLE_CONN_HANDLE_INVALID != gBLE->conn_handle)
		sd_ble_gap_disconnect(gBLE->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

void xs_ble_server_get_local_address(xsMachine *the)
{
    ret_code_t err_code;
    ble_gap_addr_t addr;
    
	err_code = sd_ble_gap_addr_get(&addr);
	if (NRF_SUCCESS == err_code) {
		xsmcSetArrayBuffer(xsResult, (void*)&addr.addr[0], BLE_GAP_ADDR_LEN);
	}
}

void xs_ble_server_set_device_name(xsMachine *the)
{
    ret_code_t err_code;
	ble_gap_conn_sec_mode_t sec_mode;
	const char *name = xsmcToString(xsArg(0));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, name, c_strlen(name));
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble set device name failed");

	gBLE->deviceNameSet = 1;
}

void xs_ble_server_start_advertising(xsMachine *the)
{
    ret_code_t err_code;
	ble_gap_adv_data_t adv_data;
	ble_gap_adv_params_t adv_params;
	
	AdvertisingFlags flags = xsmcToInteger(xsArg(0));
	uint16_t intervalMin = xsmcToInteger(xsArg(1));
	uint16_t intervalMax = xsmcToInteger(xsArg(2));
	uint8_t filterPolicy = xsmcToInteger(xsArg(3));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(4));
	uint32_t advertisingDataLength = xsmcGetArrayBufferLength(xsArg(4));

	c_memset(&adv_data, 0, sizeof(ble_gap_adv_data_t));
	c_memmove(&gBLE->adv_data[0], advertisingData, advertisingDataLength);
	adv_data.adv_data.p_data = &gBLE->adv_data[0];
	adv_data.adv_data.len = advertisingDataLength;
	if (xsmcTest(xsArg(5))) {
		uint8_t *scanResponseData = (uint8_t*)xsmcToArrayBuffer(xsArg(5));
		uint32_t scanResponseDataLength = xsmcGetArrayBufferLength(xsArg(5));

		c_memmove(&gBLE->scan_rsp_data[0], scanResponseData, scanResponseDataLength);
		adv_data.scan_rsp_data.p_data = &gBLE->scan_rsp_data[0];
		adv_data.scan_rsp_data.len = scanResponseDataLength;
	}

	switch(filterPolicy) {
		case kBLEAdvFilterPolicyWhitelistScans:
			filterPolicy = BLE_GAP_ADV_FP_FILTER_SCANREQ;
			break;
		case kBLEAdvFilterPolicyWhitelistConnections:
			filterPolicy = BLE_GAP_ADV_FP_FILTER_CONNREQ;
			break;
		case kBLEAdvFilterPolicyWhitelistScansConnections:
			filterPolicy = BLE_GAP_ADV_FP_FILTER_BOTH;
			break;
		default:
			filterPolicy = BLE_GAP_ADV_FP_ANY;
			break;
	}

	// If whitelist filtering is enabled, we cannot use LE Limited or General discoverable modes
	// Avoid the resulting BLE_ERROR_GAP_DISCOVERABLE_WITH_WHITELIST error by ensuring these modes are not enabled
	// https://devzone.nordicsemi.com/f/nordic-q-a/33606/updating-advertising-data-manuf-spec-data-in-sdk15/133504#133504
	// @@ TBD - Try enforcing this at the app level or binding layer if the behavior is portable
	if (BLE_GAP_ADV_FP_ANY != filterPolicy) {
		uint16_t i = 0;
		uint8_t *adv_ptr = &gBLE->adv_data[0];
		uint8_t *adv_end = adv_ptr + advertisingDataLength;
		while (adv_ptr < adv_end) {
			uint8_t adv_length = adv_ptr[0];
			uint8_t adv_type = adv_ptr[1];
			if (BLE_GAP_AD_TYPE_FLAGS == adv_type) {
				adv_ptr[2] &= ~(BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE | BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE);
				break;
			}
			adv_ptr += adv_length;
		}
	}
	
    c_memset(&adv_params, 0, sizeof(ble_gap_adv_params_t));
    adv_params.filter_policy   = filterPolicy;
    adv_params.interval        = intervalMin;

	if (flags & (LE_LIMITED_DISCOVERABLE_MODE | LE_GENERAL_DISCOVERABLE_MODE))
		adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	else
		adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;

	
	if (xsmcTest(xsArg(6))) {
		uint8_t notify = xsmcToBoolean(xsArg(6));
		if (notify) {
			gBLE->adv_notify = 1;
			ble_radio_notification_init(APP_IRQ_PRIORITY_LOW, NRF_RADIO_NOTIFICATION_DISTANCE_800US, ble_radio_evt);
		}
	}
	err_code = sd_ble_gap_adv_set_configure(&gBLE->adv_handle, &adv_data, &adv_params);
	if (NRF_SUCCESS == err_code)
		err_code = sd_ble_gap_adv_start(gBLE->adv_handle, APP_BLE_CONN_CFG_TAG);
	
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble start advertising failed");
}
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
    ret_code_t err_code;
    uint8_t notify = gBLE->adv_notify;
    
    gBLE->adv_notify = 0;
    
	err_code = sd_ble_gap_adv_stop(gBLE->adv_handle);
	if (NRF_ERROR_INVALID_STATE != err_code)
		xsUnknownError("ble stop advertising failed");

	if (notify)
		ble_radio_notification_deinit();
}

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
	uint16_t handle = xsmcToInteger(xsArg(0));
	uint16_t notify = xsmcToInteger(xsArg(1));
	ble_gatts_hvx_params_t hvx_params;
	uint16_t hvx_len = xsmcGetArrayBufferLength(xsArg(2));
	ret_code_t err_code;

	c_memset(&hvx_params, 0, sizeof(hvx_params));
	hvx_params.handle = handle;
	hvx_params.type   = (notify ? BLE_GATT_HVX_NOTIFICATION : BLE_GATT_HVX_INDICATION);
	hvx_params.offset = 0;
	hvx_params.p_len  = &hvx_len;
	hvx_params.p_data = xsmcToArrayBuffer(xsArg(2));
	err_code = sd_ble_gatts_hvx(gBLE->conn_handle, &hvx_params);
#if LOG_GATTS
	if (NRF_SUCCESS != err_code) {
		LOG_GATTS_MSG("sd_ble_gatts_hvx failed, err_code =");
		LOG_GATTS_INT(err_code);
	}
#endif
}

void xs_ble_server_deploy(xsMachine *the)
{
    ret_code_t err_code = NRF_SUCCESS;
    ble_uuid128_t ble_uuid_128;
    ble_add_char_params_t add_char_params;
    ble_add_descr_params_t add_desc_params;
	ble_add_char_user_desc_t add_user_desc;
	ble_gatts_char_pf_t char_presentation_format;
	uint8_t permissions;
	uint16_t properties;
	uint16_t char_handle;
	uint16_t att_handle_index = 0;
					
	if (!gBLE->deployServices) return;
	
	for (uint16_t i = 0; i < service_count; ++i) {
		gatts_attr_db_t *gatts_attr_db = (gatts_attr_db_t*)&gatt_db[i][0];
		attr_desc_t *att_desc = (attr_desc_t*)&gatts_attr_db->att_desc;
		ble_uuid_t ble_service_uuid;

		if (UUID_LEN_16 == att_desc->uuid_length && 0x00 == att_desc->value[0] && 0x18 == att_desc->value[1])
			continue;	// don't register gap service
			
		if (UUID_LEN_16 == att_desc->length) {
			uint16_t service_uuid = *(uint16_t*)att_desc->value;
			BLE_UUID_BLE_ASSIGN(ble_service_uuid, service_uuid);
		}
		else if (UUID_LEN_128 == att_desc->length) {
			uint8_t uuid_type;
			ble_uuid_128 = *(ble_uuid128_t*)att_desc->value;
			err_code = sd_ble_uuid_vs_add(&ble_uuid_128, &uuid_type);
			if (NRF_SUCCESS != err_code)
				goto bail;
				
			ble_service_uuid.type = uuid_type;
			ble_service_uuid.uuid = (att_desc->value[13] << 8) | att_desc->value[12];
		}
		else
			xsUnknownError("unsupported uuid size");

		err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_service_uuid, &service_handles[i]);
		if (NRF_SUCCESS != err_code)
			goto bail;

		for (uint16_t j = 1; j < attribute_counts[i]; ++j) {
			gatts_attr_db = (gatts_attr_db_t*)&gatt_db[i][j];
			att_desc = (attr_desc_t*)&gatts_attr_db->att_desc;

			if (0 == c_memcmp(att_desc->uuid_p, &character_declaration_uuid, UUID_LEN_16)) {
				uint8_t attIndexUserDescription = 0;
				uint8_t attIndexPresentationFormat = 0;
				properties = *att_desc->value;
				
				++j;	// advance to characteristic value
				gatts_attr_db = (gatts_attr_db_t*)&gatt_db[i][j];
				att_desc = (attr_desc_t*)&gatts_attr_db->att_desc;
				permissions = att_desc->perm;
				
				c_memset(&add_char_params, 0, sizeof(add_char_params));
				if (UUID_LEN_128 == att_desc->uuid_length) {
					add_char_params.uuid_type	= ble_service_uuid.type;
					add_char_params.uuid		= (att_desc->uuid_p[13] << 8) | att_desc->uuid_p[12];
				}
				else {
					add_char_params.uuid		= *(uint16_t*)att_desc->uuid_p;
				}
				add_char_params.max_len			= att_desc->max_length;
				add_char_params.init_len		= att_desc->length;
				add_char_params.p_init_value	= att_desc->value;
				add_char_params.is_var_len		= (NULL == att_desc->value);
				add_char_params.is_defered_read = (NULL == att_desc->value);
				add_char_params.is_defered_write = (NULL == att_desc->value);
				
				add_char_params.char_ext_props.wr_aux = (properties & GATT_CHAR_PROP_BIT_EXT_PROP ? 1 : 0);
				setAttributeProperties(&add_char_params.char_props, properties);
				setAttributePermissions(&add_char_params.read_access, &add_char_params.write_access, &add_char_params.cccd_write_access, permissions);

				att_handles[att_handle_index].service_index = i;
				att_handles[att_handle_index].att_index = j;
				
				// Add characteristic user description and/or presentation format descriptors
				for (uint16_t k = j + 1; k < attribute_counts[i]; ++k) {
					gatts_attr_db = (gatts_attr_db_t*)&gatt_db[i][k];
					att_desc = (attr_desc_t*)&gatts_attr_db->att_desc;
					
					if (0 == c_memcmp(att_desc->uuid_p, &character_declaration_uuid, UUID_LEN_16)) {
						break;
					}
					else if (0 == c_memcmp(att_desc->uuid_p, &character_user_descriptor_uuid, UUID_LEN_16)) {
						uint16_t user_desc_properties = 0;
						permissions = att_desc->perm;
						add_user_desc.max_size = att_desc->max_length;
						add_user_desc.size = att_desc->length;
						
						// Inherit properties from characteristic
						if (properties & GATT_CHAR_PROP_BIT_READ)
							user_desc_properties |= GATT_CHAR_PROP_BIT_READ;
						if (properties & GATT_CHAR_PROP_BIT_WRITE)
							user_desc_properties |= GATT_CHAR_PROP_BIT_READ;
							
						// Unlike other attributes, the user description descriptor appears to require
						// non-deferred reads and writes.
						add_user_desc.p_char_user_desc = att_desc->value;
						add_user_desc.is_var_len = 1;
						add_user_desc.is_defered_read = 0;
						add_user_desc.is_defered_write = 0;
						add_user_desc.is_value_user = 1;
						setAttributeProperties(&add_user_desc.char_props, user_desc_properties);
						setAttributePermissions(&add_user_desc.read_access, &add_user_desc.write_access, NULL, permissions);
						
						add_char_params.p_user_descr = &add_user_desc;
						attIndexUserDescription = k;
					}
					else if (0 == c_memcmp(att_desc->uuid_p, &character_presentation_format_uuid, UUID_LEN_16)) {
						char_presentation_format.format = att_desc->value[0];
						char_presentation_format.exponent = att_desc->value[1];
						char_presentation_format.unit = (att_desc->value[2] << 8) | att_desc->value[3];
						char_presentation_format.name_space = att_desc->value[4];
						char_presentation_format.desc = (att_desc->value[5] << 8) | att_desc->value[6];

						add_char_params.p_presentation_format = &char_presentation_format;
						attIndexPresentationFormat = k;
					}
				}

				err_code = characteristic_add(service_handles[i], &add_char_params, &att_handles[att_handle_index].handles);
				if (NRF_SUCCESS != err_code)
					goto bail;

				char_handle = att_handles[att_handle_index].handles.value_handle;
				
				// If the user description descriptor was added by characteristic_add() above,
				// add an att_handles entry for this descriptor so we can match this descriptor by name
				// on subsequent read and write operations.
				if (BLE_GATT_HANDLE_INVALID != att_handles[att_handle_index].handles.user_desc_handle) {
					++att_handle_index;
					c_memset(&att_handles[att_handle_index].handles, 0, sizeof(ble_gatts_char_handles_t));
					att_handles[att_handle_index].handles.value_handle = att_handles[att_handle_index - 1].handles.user_desc_handle;
					att_handles[att_handle_index].service_index = i;
					att_handles[att_handle_index].att_index = attIndexUserDescription;
				}
				
				// If the presentation format descriptor was added by characteristic_add() above,
				// there doesn't seem to be a way to find the presentation format descriptor handle.
				// Create an empty att_handles entry for this descriptor.
				if (0 != attIndexPresentationFormat) {
					++att_handle_index;
					c_memset(&att_handles[att_handle_index].handles, 0, sizeof(ble_gatts_char_handles_t));
					att_handles[att_handle_index].service_index = i;
					att_handles[att_handle_index].att_index = attIndexPresentationFormat;
				}

				++att_handle_index;
				
				// Add other descriptors
				for (uint16_t k = j + 1; k < attribute_counts[i]; ++k) {
					gatts_attr_db = (gatts_attr_db_t*)&gatt_db[i][k];
					att_desc = (attr_desc_t*)&gatts_attr_db->att_desc;
					if (0 == c_memcmp(att_desc->uuid_p, &character_declaration_uuid, UUID_LEN_16)) {
						j = k - 1;	// will be incremented by main loop
						break;
					}
					else if (0 == c_memcmp(att_desc->uuid_p, &character_client_config_uuid, UUID_LEN_16)) {
						// CCCD attributes are automatically added by characteristic_add().
					}
					else if (0 == c_memcmp(att_desc->uuid_p, &character_extended_properties_uuid, UUID_LEN_16)) {
						// Characteristic extended property descriptors are automatically added by characteristic_add().
					} 
					else if (0 == c_memcmp(att_desc->uuid_p, &character_user_descriptor_uuid, UUID_LEN_16)) {
						// Characteristic user description descriptors are added above.
					} 
					else if (0 == c_memcmp(att_desc->uuid_p, &character_presentation_format_uuid, UUID_LEN_16)) {
						// Characteristic presentation format descriptors are added above.
					}
					else {
						permissions = att_desc->perm;
						c_memset(&add_desc_params, 0, sizeof(add_desc_params));
						if (UUID_LEN_128 == att_desc->uuid_length) {
							add_desc_params.uuid_type	= ble_service_uuid.type;
							add_desc_params.uuid		= (att_desc->uuid_p[13] << 8) | att_desc->uuid_p[12];
						}
						else {
							add_desc_params.uuid		= *(uint16_t*)att_desc->uuid_p;
						}
						add_desc_params.max_len			= att_desc->max_length;
						add_desc_params.init_len		= att_desc->length;
						add_desc_params.p_value			= att_desc->value;
						add_desc_params.is_var_len		= (NULL == att_desc->value);
						add_desc_params.is_defered_read	= (NULL == att_desc->value);
						setAttributePermissions(&add_desc_params.read_access, &add_desc_params.write_access, NULL, permissions);

						att_handles[att_handle_index].service_index = i;
						att_handles[att_handle_index].att_index = j;
						
						err_code = descriptor_add(char_handle, &add_desc_params, &att_handles[att_handle_index].handles.value_handle);
						if (NRF_SUCCESS != err_code)
							goto bail;
							
						++att_handle_index;
					}
				}
			}
		}
	}
	
bail:
	if (NRF_SUCCESS != err_code)
		xsUnknownError("services deploy failed");
}

void xs_ble_server_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint8_t iocap = xsmcToInteger(xsArg(3));
	uint16_t err;
	
	gBLE->encryption = encryption;
	gBLE->bonding = bonding;
	gBLE->mitm = mitm;
	gBLE->iocap = iocap;

#if (NRF_BLE_LESC_ENABLED == 1)
	if (mitm && (iocap == DisplayYesNo))
		modLog("# warning: LE secure connections require a Nordic SDK patch. Refer to the Moddable nRF52 README.md for details.");
#endif

	err = modBLESetSecurityParameters(encryption, bonding, mitm, iocap);
	if (NRF_SUCCESS != err)
		xsUnknownError("invalid security params");
}

void xs_ble_server_passkey_input(xsMachine *the)
{
//	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t digits = xsmcToInteger(xsArg(1));
	char passkey[BLE_GAP_PASSKEY_LEN + 1];
	
	itoa(digits, passkey, 10);
	sd_ble_gap_auth_key_reply(gBLE->conn_handle, BLE_GAP_AUTH_KEY_TYPE_PASSKEY, passkey);
}

void xs_ble_server_passkey_reply(xsMachine *the)
{
	//uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	sd_ble_gap_auth_key_reply(gBLE->conn_handle, (confirm ? BLE_GAP_AUTH_KEY_TYPE_PASSKEY : BLE_GAP_AUTH_KEY_TYPE_NONE), NULL);
}

void xs_ble_server_get_service_attributes(xsMachine *the)
{
	ret_code_t err_code;
	uint16_t serviceIndex;
	uint8_t found = false;
	uint16_t length = xsmcGetArrayBufferLength(xsArg(0));
	uint8_t *buffer = xsmcToArrayBuffer(xsArg(0));

	xsmcVars(2);
	xsmcSetNewArray(xsResult, 0);

	for (serviceIndex = 0; !found && (serviceIndex < service_count); ++serviceIndex) {
		const gatts_attr_db_t *attr = &gatt_db[serviceIndex][0];
		if (attr->att_desc.length == length && 0 == c_memcmp(buffer, attr->att_desc.value, length)) {
			found = true;
			break;
		}
	}
	if (!found) return;

	for (uint16_t i = 0; i < handles_count; ++i) {
		ble_uuid_t uuid;
		ble_gatts_value_t gatts_value;
		uint16_t handle;
		
		if (serviceIndex != att_handles[i].service_index)
			continue;
			
		handle = att_handles[i].handles.value_handle;
		if (BLE_GATT_HANDLE_INVALID == handle)
			continue;
		
		xsmcSetNewObject(xsVar(0));
		xsmcSetInteger(xsVar(1), handle);
		xsmcSet(xsVar(0), xsID_handle, xsVar(1));
			
		err_code = sd_ble_gatts_attr_get(handle, &uuid, NULL);
		if (NRF_SUCCESS == err_code) {
			uint8_t buffer[UUID_LEN_128];
			uint16_t uuid_length;
			uuidToBuffer(buffer, &uuid, &uuid_length);
			xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
		}
		err_code = sd_ble_gatts_value_get(gBLE->conn_handle, handle, &gatts_value);
		if (NRF_SUCCESS == err_code && 0 != gatts_value.len) {
			xsmcSetArrayBuffer(xsVar(1), (uint8_t*)gatts_value.p_value, gatts_value.len);
			xsmcSet(xsVar(0), xsID_value, xsVar(1));
		}

		for (uint16_t j = 0; j < char_name_count; ++j) {
			if (att_handles[i].service_index == char_names[j].service_index && att_handles[i].att_index == char_names[j].att_index) {
				xsmcSetString(xsVar(1), (char*)char_names[j].name);
				xsmcSet(xsVar(0), xsID_name, xsVar(1));
				xsmcSetString(xsVar(1), (char*)char_names[j].type);
				xsmcSet(xsVar(0), xsID_type, xsVar(1));
				break;
			}
		}
			
		xsCall1(xsResult, xsID_push, xsVar(0));
	}
}

void setAttributePermissions(security_req_t *read_access, security_req_t *write_access, security_req_t *cccd_write_access, uint8_t permissions)
{
	security_req_t security_req;
	if (NULL != read_access) {
		*read_access = (permissions & GATT_PERM_READ_ENCRYPTED ? (gBLE->mitm ? SEC_MITM : SEC_JUST_WORKS) : SEC_OPEN);
	}
	if (NULL != write_access) {
		*write_access = (permissions & GATT_PERM_WRITE_ENCRYPTED ? (gBLE->mitm ? SEC_MITM : SEC_JUST_WORKS) : SEC_OPEN);
	}
	if (NULL != cccd_write_access) {
		*cccd_write_access = (permissions & GATT_PERM_WRITE_ENCRYPTED ? (gBLE->mitm ? SEC_MITM : SEC_JUST_WORKS) : SEC_OPEN);
	}
}

void setAttributeProperties(ble_gatt_char_props_t *char_props, uint16_t properties)
{
	char_props->read = (properties & GATT_CHAR_PROP_BIT_READ ? 1 : 0);
	char_props->write_wo_resp = (properties & GATT_CHAR_PROP_BIT_WRITE_NR ? 1 : 0);
	char_props->write = (properties & GATT_CHAR_PROP_BIT_WRITE ? 1 : 0);
	char_props->notify = (properties & GATT_CHAR_PROP_BIT_NOTIFY ? 1 : 0);
	char_props->indicate = (properties & GATT_CHAR_PROP_BIT_INDICATE ? 1 : 0);
}

const char_name_table *handleToCharName(uint16_t handle) {
	for (uint16_t i = 0; i < handles_count; ++i) {
		ble_gatts_char_handles_t *handles = &att_handles[i].handles;
		if (handle == att_handles[i].handles.value_handle) {
			for (uint16_t k = 0; k < char_name_count; ++k) {
				if (char_names[k].service_index == att_handles[i].service_index && char_names[k].att_index == att_handles[i].att_index)
					return &char_names[k];
			}
		}
	}
	return NULL;
}

void modBLEServerBondingRemoved(ble_gap_addr_t *peer_addr)
{
	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), peer_addr->addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), peer_addr->addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onBondingDeleted"), xsVar(0));
	xsEndHost(gBLE->the);
}

void bleServerReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	xsBeginHost(the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(the);

	// Set appearance from app GAP service when available
	// Set device name from GAP service if app hasn't already set device name in onReady() callback
	ble_gap_conn_sec_mode_t sec_mode;
	char *device_name = NULL;
	uint16_t appearance = 128;	// generic computer
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	for (uint16_t i = 0; i < service_count; ++i) {
		gatts_attr_db_t *gatts_attr_db = (gatts_attr_db_t*)&gatt_db[i];
		attr_desc_t *att_desc = (attr_desc_t*)&gatts_attr_db->att_desc;
		if (UUID_LEN_16 == att_desc->uuid_length && 0x00 == att_desc->value[0] && 0x18 == att_desc->value[1]) {
			for (uint16_t j = 1; j < attribute_counts[i]; ++j) {
				att_desc = &gatts_attr_db[j].att_desc;
				if (0 == gBLE->deviceNameSet && UUID_LEN_16 == att_desc->uuid_length && 0x2A00 == *(uint16_t*)att_desc->uuid_p) {
					device_name = c_calloc(1, att_desc->length + 1);
					c_memmove(device_name, att_desc->value, att_desc->length);
				}
				if (UUID_LEN_16 == att_desc->uuid_length && 0x2A01 == *(uint16_t*)att_desc->uuid_p) {
					appearance = att_desc->value[1] << 8 | att_desc->value[0];
				}
			}
			break;
		}
	}
	
	sd_ble_gap_appearance_set(appearance);
	
	if (NULL != device_name) {
		sd_ble_gap_device_name_set(&sec_mode, device_name, c_strlen(device_name));
		c_free(device_name);
	}
	else if (0 == gBLE->deviceNameSet) {
		sd_ble_gap_device_name_set(&sec_mode, DEVICE_FRIENDLY_NAME, c_strlen(DEVICE_FRIENDLY_NAME));
	}
}

void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modBLE ble = refcon;
	xs_ble_server_destructor(ble);
}

void gapConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	int16_t conn_handle = gap_evt->conn_handle;
	xsBeginHost(gBLE->the);
	if (BLE_CONN_HANDLE_INVALID != gBLE->conn_handle)
		goto bail;
	gBLE->conn_handle = conn_handle;
	gBLE->remote_bda = gap_evt->params.connected.peer_addr;
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.addr, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), gBLE->remote_bda.addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

    ret_code_t err_code;
	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	int16_t conn_handle = gap_evt->conn_handle;
	
	if (conn_handle != gBLE->conn_handle) return;
	xsBeginHost(gBLE->the);
	gBLE->conn_handle = BLE_CONN_HANDLE_INVALID;
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.addr, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), gBLE->remote_bda.addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDisconnected"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gapAuthKeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_auth_key_request_t const * p_evt_auth_key_request = (ble_gap_evt_auth_key_request_t const *)message;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.addr, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyInput"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gapPasskeyDisplayEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_passkey_display_t const * p_evt_passkey_display = (ble_gap_evt_passkey_display_t const *)message;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.addr, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), atoi(p_evt_passkey_display->passkey));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString(p_evt_passkey_display->match_request ? "onPasskeyConfirm" : "onPasskeyDisplay"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gattsWriteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	const char_name_table *char_name;
	ble_uuid_t uuid;
	uint16_t uuid_length;
	uint8_t buffer[UUID_LEN_128];
	ble_gatts_evt_write_t const * p_evt_write = (ble_gatts_evt_write_t const *)message;
	uint8_t notify = 0xFF;
	uint16_t handle = BLE_GATT_HANDLE_INVALID;

	for (uint16_t i = 0; BLE_GATT_HANDLE_INVALID == handle && i < handles_count; ++i) {
		if (p_evt_write->handle == att_handles[i].handles.cccd_handle && p_evt_write->len == 2) {
			notify = ble_srv_is_notification_enabled(p_evt_write->data);
			handle = p_evt_write->handle - 1;
		}
		else if (p_evt_write->handle == att_handles[i].handles.value_handle) {
			handle = p_evt_write->handle;
		}
	}
	if (BLE_GATT_HANDLE_INVALID == handle)
		return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	char_name = handleToCharName(handle);
	sd_ble_gatts_attr_get(handle, &uuid, NULL);
	uuidToBuffer(buffer, &uuid, &uuid_length);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(1), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(1));
		xsmcSetString(xsVar(1), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(1));
	}
	xsmcSetInteger(xsVar(1), handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	if (0xFF == notify) {
		xsmcSetArrayBuffer(xsVar(1), (uint8_t*)p_evt_write->data, p_evt_write->len);
		xsmcSet(xsVar(0), xsID_value, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicWritten"), xsVar(0));
	}
	else {
		xsmcSetInteger(xsVar(1), notify);
		xsmcSet(xsVar(0), xsID_notify, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsString(0 == notify ? "onCharacteristicNotifyDisabled" : "onCharacteristicNotifyEnabled"), xsVar(0));
	}
	
bail:
	xsEndHost(gBLE->the);
}

void gattsReadAuthRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gatts_evt_read_t const * read = (ble_gatts_evt_read_t const *)message;
	const char_name_table *char_name;
	uint8_t buffer[UUID_LEN_128];
	uint16_t uuid_length;

	char_name = handleToCharName(read->handle);

	uuidToBuffer(buffer, (ble_uuid_t *)&read->uuid, &uuid_length);

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSetInteger(xsVar(1), read->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(1), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(1));
		xsmcSetString(xsVar(1), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(1));
	}
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicRead"), xsVar(0));
	if (xsUndefinedType != xsmcTypeOf(xsResult)) {
		ble_gatts_rw_authorize_reply_params_t auth_reply;
		c_memset(&auth_reply, 0, sizeof(auth_reply));
		auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
		auth_reply.params.read.gatt_status = BLE_GATT_STATUS_SUCCESS;
		auth_reply.params.read.update = 1;
		auth_reply.params.read.len = xsmcGetArrayBufferLength(xsResult);
		auth_reply.params.read.p_data = xsmcToArrayBuffer(xsResult);
		sd_ble_gatts_rw_authorize_reply(gBLE->conn_handle, &auth_reply);
	}
	xsEndHost(gBLE->the);
}

void gattsWriteAuthRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gatts_evt_write_t const * write = (ble_gatts_evt_write_t const *)write;
	const char_name_table *char_name;
	uint8_t buffer[UUID_LEN_128];
	uint16_t uuid_length;

	char_name = handleToCharName(write->handle);

	uuidToBuffer(buffer, (ble_uuid_t *)&write->uuid, &uuid_length);

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSetInteger(xsVar(1), write->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(1), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(1));
		xsmcSetString(xsVar(1), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(1));
	}
	xsmcSetArrayBuffer(xsVar(1), (uint8_t*)write->data, write->len);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicWritten"), xsVar(0));
	
	ble_gatts_rw_authorize_reply_params_t auth_reply;
	c_memset(&auth_reply, 0, sizeof(auth_reply));
	auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
	auth_reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
	auth_reply.params.write.update = 1;
	auth_reply.params.write.len = write->len;
	auth_reply.params.read.p_data = write->data;
	sd_ble_gatts_rw_authorize_reply(gBLE->conn_handle, &auth_reply);

	xsEndHost(gBLE->the);
}

static void gattsMTUExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	uint16_t mtu = *(uint16_t *)message;
	xsBeginHost(gBLE->the);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), mtu);
	xsCall2(gBLE->obj, xsID_callback, xsString("onMTUExchanged"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void pmConnSecSucceededEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	pm_evt_t *pm_evt = (pm_evt_t*)message;
	pm_conn_secured_evt_t const * conn_sec_succeeded = (pm_conn_secured_evt_t const *)&pm_evt->params.conn_sec_succeeded;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	
	// The PM_CONN_SEC_PROCEDURE_ENCRYPTION procedure uses a LTK that was shared during a previous bonding procedure to encrypt the link.
	// We therefore consider consider the connection bonded when completing the PM_CONN_SEC_PROCEDURE_BONDING or PM_CONN_SEC_PROCEDURE_ENCRYPTION
	// security procedures.
	uint8_t bonded = (PM_CONN_SEC_PROCEDURE_ENCRYPTION == conn_sec_succeeded->procedure || PM_CONN_SEC_PROCEDURE_BONDING == conn_sec_succeeded->procedure);
	xsmcSetBoolean(xsVar(1), bonded);
	xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onAuthenticated"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void bleRadioOffEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onAdvertisementSent"));
	xsEndHost(gBLE->the);
}

static void ble_radio_notification_deinit()
{
	sd_nvic_ClearPendingIRQ(SWI1_IRQn);
	sd_nvic_DisableIRQ(SWI1_IRQn);
	sd_radio_notification_cfg_set(NRF_RADIO_NOTIFICATION_TYPE_NONE, NRF_RADIO_NOTIFICATION_DISTANCE_NONE);
}

static void logGAPEvent(uint16_t evt_id) {
	switch(evt_id) {
		case BLE_GAP_EVT_CONNECTED: modLog("BLE_GAP_EVT_CONNECTED"); break;
		case BLE_GAP_EVT_DISCONNECTED: modLog("BLE_GAP_EVT_DISCONNECTED"); break;
		case BLE_GAP_EVT_CONN_PARAM_UPDATE: modLog("BLE_GAP_EVT_CONN_PARAM_UPDATE"); break;
		case BLE_GAP_EVT_SEC_PARAMS_REQUEST: modLog("BLE_GAP_EVT_SEC_PARAMS_REQUEST"); break;
		case BLE_GAP_EVT_SEC_INFO_REQUEST: modLog("BLE_GAP_EVT_SEC_INFO_REQUEST"); break;
		case BLE_GAP_EVT_PASSKEY_DISPLAY: modLog("BLE_GAP_EVT_PASSKEY_DISPLAY"); break;
		case BLE_GAP_EVT_KEY_PRESSED: modLog("BLE_GAP_EVT_KEY_PRESSED"); break;
		case BLE_GAP_EVT_AUTH_KEY_REQUEST: modLog("BLE_GAP_EVT_AUTH_KEY_REQUEST"); break;
		case BLE_GAP_EVT_LESC_DHKEY_REQUEST: modLog("BLE_GAP_EVT_LESC_DHKEY_REQUEST"); break;
		case BLE_GAP_EVT_AUTH_STATUS: modLog("BLE_GAP_EVT_AUTH_STATUS"); break;
		case BLE_GAP_EVT_CONN_SEC_UPDATE: modLog("BLE_GAP_EVT_CONN_SEC_UPDATE"); break;
		case BLE_GAP_EVT_TIMEOUT: modLog("BLE_GAP_EVT_TIMEOUT"); break;
		case BLE_GAP_EVT_RSSI_CHANGED: modLog("BLE_GAP_EVT_RSSI_CHANGED"); break;
		case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: modLog("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST"); break;
		case BLE_GAP_EVT_SCAN_REQ_REPORT: modLog("BLE_GAP_EVT_SCAN_REQ_REPORT"); break;
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST: modLog("BLE_GAP_EVT_PHY_UPDATE_REQUEST"); break;
		case BLE_GAP_EVT_PHY_UPDATE: modLog("BLE_GAP_EVT_PHY_UPDATE"); break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: modLog("BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST"); break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE: modLog("BLE_GAP_EVT_DATA_LENGTH_UPDATE"); break;
		case BLE_GAP_EVT_QOS_CHANNEL_SURVEY_REPORT: modLog("BLE_GAP_EVT_QOS_CHANNEL_SURVEY_REPORT"); break;
		case BLE_GAP_EVT_ADV_SET_TERMINATED: modLog("BLE_GAP_EVT_ADV_SET_TERMINATED"); break;
	}
}

static void logGATTSEvent(uint16_t evt_id) {
	switch(evt_id) {
        case BLE_GATTS_EVT_WRITE: modLog("BLE_GATTS_EVT_WRITE"); break;
        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: modLog("BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST"); break;
        case BLE_GATTS_EVT_SYS_ATTR_MISSING: modLog("BLE_GATTS_EVT_SYS_ATTR_MISSING"); break;
        case BLE_GATTS_EVT_HVC: modLog("BLE_GATTS_EVT_HVC"); break;
        case BLE_GATTS_EVT_SC_CONFIRM: modLog("BLE_GATTS_EVT_SC_CONFIRM"); break;
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: modLog("BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST"); break;
        case BLE_GATTS_EVT_TIMEOUT: modLog("BLE_GATTS_EVT_TIMEOUT"); break;
		case BLE_GATTS_EVT_HVN_TX_COMPLETE: modLog("BLE_GATTS_EVT_HVN_TX_COMPLETE"); break;
	}
}

void ble_evt_handler(const ble_evt_t *p_ble_evt, void * p_context)
{
    uint32_t err_code;

	if (!gBLE) return;
	
	LOG_GAP_EVENT(p_ble_evt->header.evt_id);
	LOG_GATTS_EVENT(p_ble_evt->header.evt_id);
	
    switch (p_ble_evt->header.evt_id)
    {
		case BLE_GAP_EVT_CONNECTED:
			if (0xFF != gBLE->iocap)
				pm_conn_secure(p_ble_evt->evt.gap_evt.conn_handle, false);
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapConnectedEvent, NULL);
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapDisconnectedEvent, NULL);
			break;
		case BLE_GAP_EVT_AUTH_KEY_REQUEST: {
			ble_gap_evt_auth_key_request_t const * p_evt_auth_key_request = &p_ble_evt->evt.gap_evt.params.auth_key_request;
			modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt_auth_key_request, sizeof(ble_gap_evt_auth_key_request_t), gapAuthKeyRequestEvent, NULL);
			break;
		}
        case BLE_GAP_EVT_PASSKEY_DISPLAY: {
			ble_gap_evt_passkey_display_t const * p_evt_passkey_display = &p_ble_evt->evt.gap_evt.params.passkey_display;
			modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt_passkey_display, sizeof(ble_gap_evt_passkey_display_t), gapPasskeyDisplayEvent, NULL);
        	break;
        }	
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
			ble_gap_phys_t const phys = {
				.rx_phys = BLE_GAP_PHY_AUTO,
				.tx_phys = BLE_GAP_PHY_AUTO,
			};
			sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
			break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
			ble_gap_data_length_params_t dl_params;
			c_memset(&dl_params, 0, sizeof(ble_gap_data_length_params_t));
			sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &dl_params, NULL);
			break;
        }
		case BLE_GAP_EVT_SEC_INFO_REQUEST:
			sd_ble_gap_sec_info_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL, NULL, NULL);
			break;

        case BLE_GATTS_EVT_WRITE: {
			ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
			modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt_write, sizeof(ble_gatts_evt_write_t) + p_evt_write->len - 1, gattsWriteEvent, NULL);
        	break;
        }
        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: {
			ble_gatts_evt_rw_authorize_request_t const * p_evt_authorize_request = &p_ble_evt->evt.gatts_evt.params.authorize_request;
			if (BLE_GATTS_AUTHORIZE_TYPE_READ == p_evt_authorize_request->type)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&p_evt_authorize_request->request.read, sizeof(ble_gatts_evt_read_t), gattsReadAuthRequestEvent, NULL);
			else if (BLE_GATTS_AUTHORIZE_TYPE_WRITE == p_evt_authorize_request->type)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&p_evt_authorize_request->request.write, sizeof(ble_gatts_evt_write_t) + p_evt_authorize_request->request.write.len - 1, gattsWriteAuthRequestEvent, NULL);
			break;
        }
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
            uint16_t const mtu_requested = p_ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;
            uint16_t mtu_reply = (NRF_SDH_BLE_GATT_MAX_MTU_SIZE > mtu_requested ? mtu_requested : NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
            err_code = sd_ble_gatts_exchange_mtu_reply(gBLE->conn_handle, mtu_reply);
#if LOG_GATTS
			if (NRF_SUCCESS != err_code) {
				LOG_GAP_MSG("BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST failed, err_code =");
				LOG_GAP_INT(err_code);
			}
#endif
			modMessagePostToMachine(gBLE->the, (uint8_t*)&mtu_reply, sizeof(mtu_reply), gattsMTUExchangedEvent, NULL);
        	break;
        }
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
			sd_ble_gatts_sys_attr_set(gBLE->conn_handle, NULL, 0, 0);
            break;
        default:
            break;
    }
}

void ble_radio_evt(bool active)
{
	if (!active && gBLE && gBLE->adv_notify)
		modMessagePostToMachineFromISR(gBLE->the, bleRadioOffEvent, NULL);
}

static void logPMEvent(uint16_t evt_id) {
	switch(evt_id) {
		case PM_EVT_BONDED_PEER_CONNECTED: modLog("PM_EVT_BONDED_PEER_CONNECTED"); break;
		case PM_EVT_CONN_SEC_START: modLog("PM_EVT_CONN_SEC_START"); break;
		case PM_EVT_CONN_SEC_SUCCEEDED: modLog("PM_EVT_CONN_SEC_SUCCEEDED"); break;
		case PM_EVT_CONN_SEC_FAILED: modLog("PM_EVT_CONN_SEC_FAILED"); break;
		case PM_EVT_CONN_SEC_CONFIG_REQ: modLog("PM_EVT_CONN_SEC_CONFIG_REQ"); break;
		case PM_EVT_CONN_SEC_PARAMS_REQ: modLog("PM_EVT_CONN_SEC_PARAMS_REQ"); break;
		case PM_EVT_STORAGE_FULL: modLog("PM_EVT_STORAGE_FULL"); break;
		case PM_EVT_ERROR_UNEXPECTED: modLog("PM_EVT_ERROR_UNEXPECTED"); break;
		case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED: modLog("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED"); break;
		case PM_EVT_PEER_DATA_UPDATE_FAILED: modLog("PM_EVT_PEER_DATA_UPDATE_FAILED"); break;
		case PM_EVT_PEER_DELETE_SUCCEEDED: modLog("PM_EVT_PEER_DELETE_SUCCEEDED"); break;
		case PM_EVT_PEER_DELETE_FAILED: modLog("PM_EVT_PEER_DELETE_FAILED"); break;
		case PM_EVT_PEERS_DELETE_SUCCEEDED: modLog("PM_EVT_PEERS_DELETE_SUCCEEDED"); break;
		case PM_EVT_PEERS_DELETE_FAILED: modLog("PM_EVT_PEERS_DELETE_FAILED"); break;
		case PM_EVT_LOCAL_DB_CACHE_APPLIED: modLog("PM_EVT_LOCAL_DB_CACHE_APPLIED"); break;
		case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED: modLog("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED"); break;
		case PM_EVT_SERVICE_CHANGED_IND_SENT: modLog("PM_EVT_SERVICE_CHANGED_IND_SENT"); break;
		case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED: modLog("PM_EVT_SERVICE_CHANGED_IND_CONFIRMED"); break;
		case PM_EVT_SLAVE_SECURITY_REQ: modLog("PM_EVT_SLAVE_SECURITY_REQ"); break;
		case PM_EVT_FLASH_GARBAGE_COLLECTED: modLog("PM_EVT_FLASH_GARBAGE_COLLECTED"); break;
		case PM_EVT_FLASH_GARBAGE_COLLECTION_FAILED: modLog("PM_EVT_FLASH_GARBAGE_COLLECTION_FAILED"); break;
	}
}

void pm_evt_handler(pm_evt_t const * p_evt)
{
	LOG_PM_EVENT(p_evt->evt_id);
	
	pm_handler_on_pm_evt(p_evt);
	pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id) {
    	case PM_EVT_CONN_SEC_FAILED:
            // Rebond if one party has lost its keys
            if (p_evt->params.conn_sec_failed.error == PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING)
				pm_conn_secure(p_evt->conn_handle, true);
    		break;
		case PM_EVT_CONN_SEC_SUCCEEDED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt, sizeof(pm_evt_t), pmConnSecSucceededEvent, NULL);
			break;
		case PM_EVT_CONN_SEC_CONFIG_REQ: {
			pm_conn_sec_config_t pm_conn_sec_config;
			pm_conn_sec_config.allow_repairing = true;
			pm_conn_sec_config_reply(p_evt->conn_handle, &pm_conn_sec_config);
			break;
		}
        default:
            break;
    }
}
