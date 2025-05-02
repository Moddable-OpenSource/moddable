#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"
#include "flash-esp32.h"

#include "esp_partition.h"
#include "app_update/include/esp_ota_ops.h"

struct xsUpdateRecord {
	esp_ota_handle_t			ota;
	const esp_partition_t		*partition;
	uint8_t						append;
};
typedef struct xsUpdateRecord xsUpdateRecord;
typedef struct xsUpdateRecord *xsUpdate;

void xs_update_destructor(void *data)
{
	xsUpdate update = data;

	if (update) {
		if (update->ota)
			esp_ota_abort(update->ota);
	}
}

void xs_update_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) 
		return;

	xsUpdate update = xsmcGetHostChunkValidate(xsThis, xs_update_destructor);
	xs_update_destructor(update);

	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_update_open(xsMachine *the)
{
	if (!xsmcHas(xsArg(0), xsID_partition))
		xsUnknownError("no partition");

	xsmcGet(xsResult, xsArg(0), xsID_partition);
	uint8_t writable;
	xsUpdateRecord update;
	update.partition = getESP32FlashPartition(the, &xsResult, &writable);
	if (!update.partition)
		xsUnknownError("partition not found");
	if (!writable)
		xsUnknownError("read-only partition");

	update.append = 1;
	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsResult, xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsResult);
		if (0 == c_strcmp(modeStr, "a"))
			;
		else if (0 == c_strcmp(modeStr, "w"))
			update.append = 0;
		else
			xsRangeError("invalid mode");
	}

	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("unsupported");

	uint32_t byteLength = OTA_SIZE_UNKNOWN;
	if (xsmcHas(xsArg(0), xsID_byteLength)) {
		xsmcGet(xsResult, xsArg(0), xsID_byteLength);
		xsNumberValue tmp = xsmcToNumber(xsResult);
		if (c_isnan(tmp) || (tmp < 0) || (tmp > 0xffffffff) || (tmp > update.partition->size))
			xsRangeError("invalid byteLength");
		byteLength = (uint32_t)tmp;
	}

	if (update.append)
		byteLength = OTA_WITH_SEQUENTIAL_WRITES;

	esp_err_t err = esp_ota_begin(update.partition, byteLength, &update.ota);
	if (ESP_OK != err)
		xsUnknownError("begin failed");

	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, (void *)&update, sizeof(update));
}

void xs_update_write(xsMachine *the)
{
	xsUpdate update = xsmcGetHostChunkValidate(xsThis, xs_update_destructor);
	esp_ota_handle_t ota = update->ota;
	void *data;
	xsUnsignedValue size;
	esp_err_t err;

	if (!update->ota)
		xsUnknownError("already complete");

	if (update->append) {
		if (xsmcArgc > 1)
			xsUnknownError("offset not allowed");
		xsmcGetBufferReadable(xsArg(0), &data, &size);
		err = esp_ota_write(ota, data, size);
	}
	else {
		xsNumberValue offset = xsmcToNumber(xsArg(1));
		if ((offset < 0) || (offset > 0xffffffff))
			xsRangeError("invalid offset");
		xsmcGetBufferReadable(xsArg(0), &data, &size);
		err = esp_ota_write_with_offset(ota, data, size, (uint32_t)offset);
	}
	if (ESP_OK != err)
		xsUnknownError("write failed");
}

void xs_update_complete(xsMachine *the)
{
	xsUpdate update = xsmcGetHostChunkValidate(xsThis, xs_update_destructor);

	if (!update->ota)
		xsUnknownError("already complete");

	esp_err_t err = esp_ota_end(update->ota);
	update->ota = 0;
	if (ESP_OK != err)
		xsUnknownError("end failed");

	err = esp_ota_set_boot_partition(update->partition);
	if (ESP_OK != err)
		xsUnknownError("can't change boot partition");
}
