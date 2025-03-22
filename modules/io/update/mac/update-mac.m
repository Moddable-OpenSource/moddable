#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"
#include "flash-mac.h"

struct xsUpdateRecord {
	xsFlash flash;
	xsUnsignedValue offset;
	xsUnsignedValue size;
	uint8_t append;
	uint8_t done;
};
typedef struct xsUpdateRecord xsUpdateRecord;
typedef struct xsUpdateRecord *xsUpdate;

void xs_update_destructor(void *it)
{
	if (it) {
		xsUpdate update = it;
		c_free(update);
	}
}

void xs_update_close(xsMachine *the)
{
	xsUpdate update = xsmcGetHostData(xsThis);
	if (update && xsmcGetHostDataValidate(xsThis, xs_update_destructor)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xs_update_destructor(update);
	}
}

void xs_update_complete(xsMachine *the)
{
	xsUpdate update = xsmcGetHostDataValidate(xsThis, xs_update_destructor);
	xsFlash flash = update->flash;
	if (update->done)
		xsUnknownError("already complete");
	update->done = 1;
	if (flash->bytes[0] != 0xF9)
		xsUnknownError("invalid image");
}

void xs_update_write(xsMachine *the)
{
	xsUpdate update = xsmcGetHostDataValidate(xsThis, xs_update_destructor);
	void *data;
	xsUnsignedValue size;

	if (update->done)
		xsUnknownError("already complete");

	if (update->append) {
		if (xsmcArgc > 1)
			xsUnknownError("offset not allowed");
		xsmcGetBufferReadable(xsArg(0), &data, &size);
	}
	else {
		xsNumberValue offset = xsmcToNumber(xsArg(1));
		if (c_isnan(offset) || (offset < 0) || (offset > 0xffffffff))
			xsRangeError("invalid offset");
		update->offset = (xsUnsignedValue)offset;
		xsmcGetBufferReadable(xsArg(0), &data, &size);
	}
	if (update->offset + size > update->size)
		xsRangeError("invalid size");
	memcpy(update->flash->bytes + update->offset, data, size);
	update->offset += size;
}

void xs_update_get_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, xs_update_destructor);
	builtinGetFormat(the, kIOFormatBuffer);
}

void xs_update_set_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, xs_update_destructor);
	uint8_t format = builtinSetFormat(the);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
}

void xs_update_open(xsMachine *the)
{
	xsFlash flash;
	uint8_t append;
	xsUnsignedValue size;
	xsUpdate update;

	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("unsupported");
	if (!xsmcHas(xsArg(0), xsID_partition))
		xsUnknownError("no partition");

	xsmcGet(xsResult, xsArg(0), xsID_partition);
	flash = xsmcGetHostDataValidate(xsResult, xs_flash_partition_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only partition");

	append = 1;
	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsResult, xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsResult);
		if (0 == c_strcmp(modeStr, "a"))
			;
		else if (0 == c_strcmp(modeStr, "w"))
			append = 0;
		else
			xsRangeError("invalid mode");
	}

	size = flash->size;
	if (xsmcHas(xsArg(0), xsID_byteLength)) {
		xsmcGet(xsResult, xsArg(0), xsID_byteLength);
		xsNumberValue tmp = xsmcToNumber(xsResult);
		if (c_isnan(tmp) || (tmp < 0) || (tmp > 0xffffffff) || (tmp > flash->size))
			xsRangeError("invalid byteLength");
		size = xsmcToInteger(xsResult);
	}
	memset(flash->bytes, -1, size);
	
	update = c_calloc(1, sizeof(xsUpdateRecord));
	if (!update)
		xsRangeError("not enough memory");
	update->flash = flash;
	update->size = size;
	update->append = append;
	
	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostData(xsResult, update);
}
