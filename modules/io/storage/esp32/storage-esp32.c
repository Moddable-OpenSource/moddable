
#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"

#include "nvs_flash/include/nvs_flash.h"

struct xsStorageIteratorRecord {
	nvs_iterator_t	it;
};
typedef struct xsStorageIteratorRecord xsStorageIteratorRecord;
typedef struct xsStorageIteratorRecord *xsStorageIterator;

struct xsDirectoryRecord {
	nvs_handle	handle;
	uint8_t		format;
	uint8_t		readOnly;
};
typedef struct xsDirectoryRecord xsDirectoryRecord;
typedef struct xsDirectoryRecord *xsDirectory;

/*
	helpers
*/

#define throwIf(a) _throwIf(the, a)

static void _throwIf(xsMachine *the, esp_err_t err)
{
	if (ESP_OK != err) {
		char msg[64];
		xsUnknownError((char *)esp_err_to_name_r(err, msg, sizeof(msg)));
	}
}

#define getDirectory(slot) ((xsDirectory)xsmcGetHostChunkValidate(slot, xs_directorystorage_destructor))->handle

/*
	Storage Iterator
*/

void xs_storageIterator_constructor(xsMachine *the)
{
	nvs_handle handle = getDirectory(xsArg(0));
	xsStorageIteratorRecord iterator = {0};

	esp_err_t err = nvs_entry_find_in_handle(handle, NVS_TYPE_ANY, &iterator.it);
	if (err && (err != ESP_ERR_NVS_NOT_FOUND))
		throwIf(err);

	xsmcSetHostChunk(xsThis, &iterator, sizeof(iterator));
}

void xs_storageIterator_destructor(void *data)
{
	xsStorageIterator iterator = data;
	if (iterator)
		nvs_release_iterator(iterator->it);
}

void xs_storageIterator_next(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostChunkValidate(xsThis, xs_storageIterator_destructor);
	nvs_iterator_t it = iterator->it;

	xsmcVars(2);
	if (it) {
		nvs_entry_info_t info;
		throwIf(nvs_entry_info(it, &info));
		esp_err_t err = nvs_entry_next(&it);
		if (!it)
			iterator->it = NULL;
		else
			throwIf(err);

		xsmcSetFalse(xsVar(0));
		xsmcSetString(xsVar(1), info.key);
	}
	else {
		xsmcSetTrue(xsVar(0));
	}

	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_storageIterator_return(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostChunkValidate(xsThis, xs_storageIterator_destructor);
	if (iterator->it) {
		xs_storageIterator_destructor(iterator);
		iterator->it = NULL;
		xs_storageIterator_next(the);
		xsmcSetHostChunk(xsThis, NULL, 0);
	}
}

/*
	Directory
*/

void xs_directorystorage_destructor(void *data)
{
	xsDirectory d = data;
	if (d && d->handle)
		nvs_close(d->handle);
}

void xs_directorystorage_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) 
		return;

	xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);

	nvs_close(getDirectory(xsThis));
	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_directorystorage_open(xsMachine *the)
{
	nvs_open_mode_t mode = NVS_READWRITE;
	char domain[64];

	xsmcVars(1);
	if (!xsmcHas(xsArg(0), xsID_path))
		xsUnknownError("no path");
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	xsmcToStringBuffer(xsVar(0), domain, sizeof(domain));
	size_t length = c_strlen(domain);
	if (0 == length) xsUnknownError("invalid");
	if ('/' == domain[length - 1])
		domain[length - 1] = 0;

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsVar(0));
		if (0 == c_strcmp(modeStr, "r"))
			mode = NVS_READONLY;
		else if (0 == c_strcmp(modeStr, "r+"))
			;
		else
			xsUnknownError("invalid mode");
	}

	xsDirectoryRecord d = {0};
	d.format = builtinInitializeFormat(the, kIOFormatBuffer);
	d.readOnly = mode == NVS_READONLY;
	if (kIOFormatInvalid == d.format)
		xsRangeError("unsupported");
	throwIf(nvs_open(domain, mode, &d.handle));

	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, &d, sizeof(d));
}

void xs_directorystorage_delete(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	char key[64];

	if (d->readOnly)
		xsUnknownError("read-only");

	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	esp_err_t err = nvs_erase_key(d->handle, key);
	if (ESP_ERR_NVS_NOT_FOUND == err) err = ESP_OK;
	throwIf(err);

	throwIf(nvs_commit(d->handle));
}

void xs_directorystorage_read(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	nvs_handle handle = d->handle;
	uint8_t format = d->format;
	char key[64];

	xsmcToStringBuffer(xsArg(0), key, sizeof(key));
	switch (format) {
		case kIOFormatBuffer: {
			void *data;
			size_t length;
			nvs_type_t type;
			esp_err_t err = nvs_find_key(handle, key, &type);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			if (NVS_TYPE_BLOB != type)
				xsTypeError("mismatch");

			nvs_get_blob(handle, key, NULL, &length);		// bug: nvs_get_blob doesn't check type before returning length. work around above with nvs_find_key
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);

			if (xsmcArgc > 1) {
				xsUnsignedValue dataLength;

				xsmcGetBufferWritable(xsArg(1), &data, &dataLength);
				if (dataLength < length)
					xsRangeError("buffer too small");
				xsmcSetInteger(xsResult, length);
			}
			else {
				xsmcSetArrayBuffer(xsResult, NULL, length);
				data = xsmcToArrayBuffer(xsResult);
			}

			throwIf(nvs_get_blob(handle, key, data, &length));
			} break;

		case kIOFormatString: {
			size_t length;
			nvs_type_t type;
			esp_err_t err = nvs_find_key(handle, key, &type);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			if (NVS_TYPE_STR != type)
				xsTypeError("mismatch");
			err = nvs_get_str(handle, key, NULL, &length);		// bug: nvs_get_str doesn't check type before returning length. work around above with nvs_find_key
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);

			xsmcSetStringBuffer(xsResult, NULL, length);
			throwIf(nvs_get_str(handle, key, xsmcToString(xsResult), &length));
			} break;

		case kIOFormatUint8: {
			uint8_t value;
			esp_err_t err = nvs_get_u8(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			xsmcSetInteger(xsResult, value);
			} break;

		case kIOFormatInt8: {
			int8_t value;
			esp_err_t err = nvs_get_i8(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			xsmcSetInteger(xsResult, value);
			} break;

		case kIOFormatUint16: {
			uint16_t value;
			esp_err_t err = nvs_get_u16(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			xsmcSetInteger(xsResult, value);
			} break;

		case kIOFormatInt16: {
			int16_t value;
			esp_err_t err = nvs_get_i16(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			xsmcSetInteger(xsResult, value);
			} break;

		case kIOFormatUint32: {
			uint32_t value;
			esp_err_t err = nvs_get_u32(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			if (value >> 31)
				xsmcSetNumber(xsResult, value);
			else
				xsmcSetInteger(xsResult, value);
			} break;

		case kIOFormatInt32: {
			int32_t value;
			esp_err_t err = nvs_get_i32(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			xsmcSetInteger(xsResult, value);
			} break;

		case kIOFormatInt64: {
			int64_t value;
			esp_err_t err = nvs_get_i64(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			fxFromBigInt64(the, &xsResult, value);
			} break;

		case kIOFormatUint64: {
			uint64_t value;
			esp_err_t err = nvs_get_u64(handle, key, &value);
			if (ESP_ERR_NVS_NOT_FOUND == err) return;
			throwIf(err);
			fxFromBigUint64(the, &xsResult, value);
			} break;

		default:
			xsUnknownError("unexpected");
			break;
	}
}

void xs_directorystorage_write(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	nvs_handle handle = d->handle;
	uint8_t format = d->format;
	char key[64];

	if (d->readOnly)
		xsUnknownError("read-only");

		//@@ ERASE_KEY here if type changes

	xsmcToStringBuffer(xsArg(0), key, sizeof(key));
	switch (format) {
		case kIOFormatBuffer: {
			void *data;
			xsUnsignedValue length;

			xsmcGetBufferReadable(xsArg(1), &data, &length);
			throwIf(nvs_set_blob(handle, key, data, length));
			} break;

		case kIOFormatString:
			throwIf(nvs_set_str(handle, key, xsmcToString(xsArg(1))));
			break;

		case kIOFormatUint8:
			throwIf(nvs_set_u8(handle, key, (uint8_t)xsmcToInteger(xsArg(1))));
			break;

		case kIOFormatInt8:
			throwIf(nvs_set_i8(handle, key, (int8_t)xsmcToInteger(xsArg(1))));
			break;
		
		case kIOFormatUint16:
			throwIf(nvs_set_u16(handle, key, (uint16_t)xsmcToInteger(xsArg(1))));
			break;

		case kIOFormatInt16:
			throwIf(nvs_set_i16(handle, key, (int16_t)xsmcToInteger(xsArg(1))));
			break;
		
		case kIOFormatUint32:
			throwIf(nvs_set_u32(handle, key, (uint32_t)xsmcToInteger(xsArg(1))));
			break;

		case kIOFormatInt32:
			throwIf(nvs_set_i32(handle, key, (int32_t)xsmcToInteger(xsArg(1))));
			break;

		case kIOFormatInt64: {
			int64_t value = (int64_t)fxToBigInt64(the, &xsArg(1));
			throwIf(nvs_set_i64(handle, key, value));
			} break;

		case kIOFormatUint64: {
			uint64_t value = (uint64_t)fxToBigUint64(the, &xsArg(1));
			throwIf(nvs_set_u64(handle, key, value));
			} break;

		default:
			xsUnknownError("unexpected");
			break;
	}

	throwIf(nvs_commit(handle));
}

void xs_directorystorage_format_get(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	builtinGetFormat(the, d->format);
}

void xs_directorystorage_format_set(xsMachine *the)
{
	static const uint8_t formats[] = {kIOFormatBuffer, kIOFormatString, kIOFormatUint8, kIOFormatInt8, kIOFormatUint16, kIOFormatInt16, kIOFormatUint32, kIOFormatInt32, kIOFormatUint64, kIOFormatInt64, kIOFormatInvalid};
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	uint8_t format = builtinSetFormat(the), i = 0;

	while (kIOFormatInvalid != formats[i]) {
		if (formats[i++] == format) {
			d->format = format;
			return;
		}
	}

	xsRangeError("unsupported");
}
