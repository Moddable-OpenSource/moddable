#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "builtinCommon.h"

struct xsStorageRecord {
	uint8_t readOnly;
	uint8_t format;
	char domain[1];
};
typedef struct xsStorageRecord xsStorageRecord;
typedef struct xsStorageRecord *xsStorage;

void xs_storage_domain_destructor(void *it)
{
	if (it)
		c_free(it);
}

void xs_storage_domain_close(xsMachine *the)
{
	xsStorage storage = xsmcGetHostData(xsThis);
	if (!storage)
		return;

	xsmcGetHostDataValidate(xsThis, xs_storage_domain_destructor);
	xs_storage_domain_destructor(storage);

	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_storage_domain_delete(xsMachine *the)
{
	xsStorage storage = xsmcGetHostDataValidate(xsThis, xs_storage_domain_destructor);
	if (storage->readOnly)
		xsUnknownError("read only");
	CFStringRef domain = CFStringCreateWithCString(NULL, storage->domain, kCFStringEncodingUTF8);
	CFStringRef key = CFStringCreateWithCString(NULL, xsmcToString(xsArg(0)), kCFStringEncodingUTF8);
	CFPreferencesSetValue(key, NULL, domain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	CFPreferencesSynchronize(domain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	CFRelease(key);
	CFRelease(domain);
}

void xs_storage_domain_read(xsMachine *the)
{
	xsStorage storage = xsmcGetHostDataValidate(xsThis, xs_storage_domain_destructor);
	CFStringRef domain = CFStringCreateWithCString(NULL, storage->domain, kCFStringEncodingUTF8);
	CFStringRef key = CFStringCreateWithCString(NULL, xsmcToString(xsArg(0)), kCFStringEncodingUTF8);
	CFTypeRef value = (CFTypeRef)CFPreferencesCopyValue(key, domain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	if (value) {
		CFTypeID type = CFGetTypeID(value);
		if (type == CFNumberGetTypeID()) {
			switch (storage->format) {
				case kIOFormatInt8: {
					int8_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt8Type, &number);
					xsResult = xsInteger(number);
				} break;
				case kIOFormatInt16: {
					int16_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt16Type, &number);
					xsResult = xsInteger(number);
				} break;
				case kIOFormatInt32: {
					int32_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt32Type, &number);
					xsResult = xsInteger(number);
				} break;
				case kIOFormatInt64: {
					int64_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt64Type, &number);
					fxFromBigInt64(the, &xsResult, number);
				} break;
				case kIOFormatUint8: {
					uint8_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt8Type, &number);
					xsResult = xsInteger(number);
				} break;
				case kIOFormatUint16: {
					uint16_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt16Type, &number);
					xsResult = xsInteger(number);
				} break;
				case kIOFormatUint32: {
					uint32_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt32Type, &number);
					fxUnsigned(the, &xsResult, number);
				} break;
				case kIOFormatUint64: {
					uint64_t number = 0;
					CFNumberGetValue(value, kCFNumberSInt64Type, &number);
					fxFromBigUint64(the, &xsResult, number);
				} break;
			}
		}
		else if (type == CFStringGetTypeID()) {
			if (kIOFormatString != storage->format)
				xsTypeError("mismatch");
			CFRange range = CFRangeMake(0, CFStringGetLength(value));
			CFIndex length;
			CFStringGetBytes(value, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &length);
			xsResult = xsStringBuffer(NULL, length);
			CFStringGetBytes(value, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)xsmcToString(xsResult), length, NULL);
		}
		else {
			if (kIOFormatBuffer != storage->format)
				xsTypeError("mismatch");
			size_t valueLength = CFDataGetLength(value);
			void* data;
			if (xsmcArgc > 1) {
				xsUnsignedValue dataLength;
				xsmcGetBufferWritable(xsArg(1), &data, &dataLength);
				if (dataLength < valueLength)
					xsRangeError("buffer too small");
				xsmcSetInteger(xsResult, valueLength);
			}
			else {
				xsmcSetArrayBuffer(xsResult, NULL, valueLength);
				data = xsmcToArrayBuffer(xsResult);
			}
        	c_memcpy(data, CFDataGetBytePtr(value), valueLength);
        }
		CFRelease(value);
	}
	CFRelease(key);
	CFRelease(domain);
}

void xs_storage_domain_write(xsMachine *the)
{
	xsStorage storage = xsmcGetHostDataValidate(xsThis, xs_storage_domain_destructor);
	if (storage->readOnly)
		xsUnknownError("read only");
	CFStringRef domain = CFStringCreateWithCString(NULL, storage->domain, kCFStringEncodingUTF8);
	CFStringRef key = CFStringCreateWithCString(NULL, xsmcToString(xsArg(0)), kCFStringEncodingUTF8);
	CFTypeRef value = NULL;
	switch (storage->format) {
		case kIOFormatBuffer: {
			void *data;
			xsUnsignedValue dataLength;
			xsmcGetBufferReadable(xsArg(1), &data, &dataLength);
			value = CFDataCreate(NULL, data, dataLength);
		} break;
		case kIOFormatString: {
			value = CFStringCreateWithCString(NULL, xsmcToString(xsArg(1)), kCFStringEncodingUTF8);
		} break;
		case kIOFormatInt8: {
			int8_t number = (int8_t)xsmcToInteger(xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt8Type, &number);
		} break;
		case kIOFormatInt16: {
			int16_t number = (int16_t)xsmcToInteger(xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt16Type, &number);
		} break;
		case kIOFormatInt32: {
			int32_t number = (int32_t)xsmcToInteger(xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt32Type, &number);
		} break;
		case kIOFormatInt64: {
			int64_t number = (int64_t)fxToBigInt64(the, &xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt64Type, &number);
		} break;
		case kIOFormatUint8: {
			uint8_t number = (uint8_t)xsmcToInteger(xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt8Type, &number);
		} break;
		case kIOFormatUint16: {
			uint16_t number = (uint16_t)xsmcToInteger(xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt16Type, &number);
		} break;
		case kIOFormatUint32: {
			uint32_t number = (uint32_t)fxToUnsigned(the, &xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt32Type, &number);
		} break;
		case kIOFormatUint64: {
			int64_t number = (int64_t)fxToBigUint64(the, &xsArg(1));
			value = CFNumberCreate(NULL, kCFNumberSInt64Type, &number);
		} break;
		default: {
			xsUnknownError("unexpected");
		} break;
	}
	CFPreferencesSetValue(key, value, domain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	CFPreferencesSynchronize(domain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	CFRelease(value);
	CFRelease(key);
	CFRelease(domain);
}

void xs_storage_domain_get_format(xsMachine *the)
{
	xsStorage storage = xsmcGetHostDataValidate(xsThis, xs_storage_domain_destructor);
	builtinGetFormat(the, storage->format);
}

void xs_storage_domain_set_format(xsMachine *the)
{
	static const uint8_t formats[] = {kIOFormatBuffer, kIOFormatString, kIOFormatUint8, kIOFormatInt8, kIOFormatUint16, kIOFormatInt16, kIOFormatUint32, kIOFormatInt32, kIOFormatUint64, kIOFormatInt64, kIOFormatInvalid};
	xsStorage storage = xsmcGetHostDataValidate(xsThis, xs_storage_domain_destructor);
	uint8_t format = builtinSetFormat(the), i = 0;
	while (kIOFormatInvalid != formats[i]) {
		if (formats[i++] == format) {
			storage->format = format;
			return;
		}
	}
	xsRangeError("unsupported format");
}

void xs_storage_domain_open(xsMachine *the)
{
	size_t baseLength = c_strlen(PIU_DOT_SIGNATURE);
	char path[64];
	xsBooleanValue readOnly = 0;
	uint8_t format;
	xsmcVars(1);
	
	if (!xsmcHas(xsArg(0), xsID_path))
		xsUnknownError("no path");
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	xsmcToStringBuffer(xsVar(0), path, sizeof(path));
	size_t pathLength = c_strlen(path);
	if (pathLength == 0)
		xsUnknownError("invalid path");
	if ('/' == path[pathLength - 1])
		pathLength--;
		
	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsVar(0));
		if (0 == c_strcmp(modeStr, "r"))
			readOnly = 1;
		else if (0 != c_strcmp(modeStr, "r+"))
			xsUnknownError("invalid mode");
	}
	
	format = builtinInitializeFormat(the, kIOFormatBuffer);
	if (kIOFormatInvalid == format)
		xsRangeError("unsupported format");
	
	xsStorage result = c_calloc(1, sizeof(xsStorageRecord) + baseLength + 1 + pathLength);
	if (!result)
		xsRangeError("not enough memory");
	result->readOnly = readOnly;	
	result->format = format;	
	c_memcpy(result->domain, PIU_DOT_SIGNATURE, baseLength);
	result->domain[baseLength] = '.';
	c_memcpy(result->domain + baseLength + 1, path, pathLength);
	
	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostData(xsResult, result);
}

struct xsStorageIteratorRecord {
	CFArrayRef list;
	CFIndex count;
	CFIndex index;
};
typedef struct xsStorageIteratorRecord xsStorageIteratorRecord;
typedef struct xsStorageIteratorRecord *xsStorageIterator;

void xs_storage_domain_key_iterator_constructor(xsMachine *the)
{
	xsStorage storage = xsmcGetHostDataValidate(xsArg(0), xs_storage_domain_destructor);
	CFStringRef domain = CFStringCreateWithCString(NULL, storage->domain, kCFStringEncodingUTF8);
	xsStorageIterator iterator = c_calloc(1, sizeof(xsStorageIteratorRecord));
	if (!iterator)
		xsRangeError("not enough memory");
	iterator->list = CFPreferencesCopyKeyList(domain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	if (iterator->list)
		iterator->count = CFArrayGetCount(iterator->list);
	xsmcSetHostData(xsThis, iterator);
	CFRelease(domain);
}

void xs_storage_domain_key_iterator_destructor(void *it)
{
	if (it) {
		xsStorageIterator iterator = it;
		if (iterator->list)
			CFRelease(iterator->list);
		c_free(it);
	}
}

void xs_storage_domain_key_iterator_next(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostDataValidate(xsThis, xs_storage_domain_key_iterator_destructor);
	xsmcVars(2);
	if (iterator->index < iterator->count) {
		CFStringRef key = CFArrayGetValueAtIndex(iterator->list, iterator->index);
		char buffer[64];
		CFStringGetCString(key, buffer, sizeof(buffer), kCFStringEncodingUTF8); 
		xsmcSetFalse(xsVar(0));
		xsmcSetString(xsVar(1), buffer);
		iterator->index++;
	}
	else {
		xsmcSetTrue(xsVar(0));
		xsmcSetUndefined(xsVar(1));
	}
	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_storage_domain_key_iterator_return(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostDataValidate(xsThis, xs_storage_domain_key_iterator_destructor);
	if (iterator->list) {
		CFRelease(iterator->list);
		iterator->list = NULL;
		iterator->index = iterator->count;
	}
	xs_storage_domain_key_iterator_next(the);
}

