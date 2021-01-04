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

#error - this modPreference implementation is deprecated and replaced by preference/esp/modPreference.c

// #include "nvs_flash/include/nvs_flash.h"

#include "nrf.h"
#include "fds.h"


/*
	NRF52 Flash Data Storage module uses 16 bit values for
	FileIDs and record keys with these restrictions when
	Peer Manager is not use (Moddable doesn't currently)

	File IDs should be in the range 0x0000 - 0xFFFE 
	Record IDs should be in the range 0x0001 - 0FFFF

	using first two characters of (domain) for FileID
	using first two characters of (key) for RecordID
*/

static void files_init(void);
static void wait_for_fds_ready(void);
static void power_manage(void);

struct prefFormat {
	uint16_t size;
	uint8_t type;
	uint8_t pad;
	uint8_t data[];
};
#define prefHeaderSize 4

typedef struct prefFormat prefFormat;

#define PREF_KIND_U8 	1
#define PREF_KIND_U32	2
#define PREF_KIND_STR	3
#define PREF_KIND_BLOB	4

static int gInitialized = 0;
static int gFDSInitialized = 0;
static int gFDSWriteFinished = 0;

static uint16_t strToFileID(uint8_t *str)
{
	uint16_t ret = 0;
	if (*str == 0) {
		ret = 0xfffe;
	}
	else
		ret = (str[0] << 8) | str[1];

	return ret;
}

static void fds_event_handler(fds_evt_t const * p_evt)
{
	if (NRF_SUCCESS == p_evt->result) {
		switch (p_evt->id) {
			case FDS_EVT_INIT:
				gFDSInitialized = 1;
				break;
			case FDS_EVT_WRITE:
				gFDSWriteFinished = 1;
				break;
			case FDS_EVT_DEL_RECORD:
				break;
			default:
				break;
		}
	}
}

void xs_preference_set(xsMachine *the)
{
	ret_code_t rc;
	uint8_t b, recordOK = 0;
	int32_t integer, size;
	uint32_t *u32p;
	uint8_t *u8p;
	char *str;
	double dbl;

	fds_record_desc_t desc = {0};
	fds_find_token_t tok = {0};
	fds_flash_record_t flashRecord = {0};
	fds_record_t prefRecord = {0};
	uint32_t domain;
	uint32_t key;

	prefFormat *pref;

	domain = strToFileID(xsmcToString(xsArg(0)));
	key = strToFileID(xsmcToString(xsArg(1)));

	files_init();

	rc = fds_record_find(domain, key, &desc, &tok);

	if (NRF_SUCCESS == rc) {
		rc = fds_record_open(&desc, &flashRecord);
		APP_ERROR_CHECK(rc);
		recordOK = 1;
		rc = fds_record_close(&desc);
	}

	switch (xsmcTypeOf(xsArg(2))) {
		case xsBooleanType:
			size = prefHeaderSize + 1;
			pref = (prefFormat*)c_calloc(size, 1);
			pref->type = PREF_KIND_U8;
			pref->size = 1;
			u8p = pref->data;
			*u8p = xsmcToBoolean(xsArg(2));
		break;

		case xsIntegerType:
			size = prefHeaderSize + 4;
			pref = (prefFormat*)c_calloc(size, 1);
			pref->type = PREF_KIND_U32;
			pref->size = 4;
			u32p = (uint32_t*)pref->data;
			*u32p = xsmcToInteger(xsArg(2));
			break;

		case xsNumberType:
			dbl = xsmcToNumber(xsArg(2));
			if (dbl != (int)dbl) {
				xsUnknownError("float unsupported");
				goto done;
			}
			size = prefHeaderSize + 4;
			pref = (prefFormat*)c_calloc(size, 1);
			pref->type = PREF_KIND_U32;
			pref->size = 4;
			u32p = (uint32_t*)pref->data;
			*u32p = xsmcToInteger(xsArg(2));
			break;

		case xsStringType:
			str = xsmcToString(xsArg(2));
			size = prefHeaderSize + c_strlen(str);
			pref = (prefFormat*)c_calloc(size, 1);
			pref->type = PREF_KIND_STR;
			pref->size = c_strlen(str);
			c_memcpy(pref->data, str, pref->size);
			break;

		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				size = prefHeaderSize + xsmcGetArrayBufferLength(xsArg(2));
				pref = (prefFormat*)c_calloc(size, 1);
				pref->type = PREF_KIND_BLOB;
				pref->size = xsmcGetArrayBufferLength(xsArg(2));
				c_memcpy(pref->data, xsmcToArrayBuffer(xsArg(2)), pref->size);
			}
			else
				goto done;
			break;

		default:
			xsUnknownError("unsupported type");
	}

	prefRecord.file_id = domain;
	prefRecord.key = key;
	prefRecord.data.p_data = pref;
	prefRecord.data.length_words = (size + 3) / 4;

	if (recordOK) {
		rc = fds_record_update(&desc, &prefRecord);
		if (NRF_SUCCESS != rc) {
			xsUnknownError("pref update error");
		}
	}
	else {
		rc = fds_record_write(NULL, &prefRecord);
		if (NRF_SUCCESS != rc) {
			xsUnknownError("pref write error");
		}
	}

done:
	// will deleting the calloc'd data above be a problem?
	// may need to wait until we receive a callback
	;
}

void xs_preference_get(xsMachine *the)
{
	ret_code_t rc;
	uint8_t b;
	int32_t integer;

	fds_record_desc_t desc = {0};
	fds_find_token_t tok = {0};
	fds_flash_record_t prefRecord = {0};
	uint32_t domain;
	uint32_t key;

	files_init();

	domain = strToFileID(xsmcToString(xsArg(0)));
	key = strToFileID(xsmcToString(xsArg(1)));

	rc = fds_record_find(domain, key, &desc, &tok);

	if (NRF_SUCCESS == rc) {
		prefFormat *pref;
		rc = fds_record_open(&desc, &prefRecord);
		APP_ERROR_CHECK(rc);
	
		pref = (prefFormat*)prefRecord.p_data;	
		if (PREF_KIND_U8 == pref->type) {
			b = pref->data[0];
			xsmcSetBoolean(xsResult, b);
		}
		else if (PREF_KIND_U32 == pref->type) {
			integer = *(uint32_t*)pref->data;
			xsmcSetInteger(xsResult, integer);
		}
		else if (PREF_KIND_STR == pref->type) {
			xsResult = xsStringBuffer(pref->data, pref->size);
		}
		else if (PREF_KIND_BLOB == pref->type) {
			xsmcSetArrayBuffer(xsResult, pref->data, pref->size);
		}
		else
			xsmcSetUndefined(xsResult);	// not an error if not found, just undefined

		rc = fds_record_close(&desc);
		APP_ERROR_CHECK(rc);
	}
	else
		xsmcSetUndefined(xsResult);	// not an error if not found, just undefined
}

void xs_preference_delete(xsMachine *the)
{
	ret_code_t rc;
	fds_record_desc_t desc = {0};
	fds_find_token_t tok = {0};
	uint32_t domain;
	uint32_t key;

	files_init();

	domain = strToFileID(xsmcToString(xsArg(0)));
	key = strToFileID(xsmcToString(xsArg(1)));

	rc = fds_record_find(domain, key, &desc, &tok);

	if (NRF_SUCCESS == rc) {
		rc = fds_record_delete(&desc);
		if (NRF_SUCCESS != rc) {
			xsUnknownError("failed to delete preference record");
		}
	}
}

void xs_preference_keys(xsMachine *the)
{
	xsResult = xsNewArray(0);
}

static void files_init()
{
	ret_code_t rc;

	if (gInitialized)
		return;
	gInitialized = 1;
	fds_register(fds_event_handler);
	rc = fds_init();
	APP_ERROR_CHECK(rc);

	wait_for_fds_ready();
}

static void power_manage(void)
{
	#ifdef SOFTDEVICE_PRESENT
		(void) sd_app_evt_wait();
	#else
		__WFE();
	#endif
}

static void wait_for_fds_ready(void)
{
	while (!gFDSInitialized) {
		power_manage();
	}
}

