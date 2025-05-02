#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"
#include "flash-esp32.h"

#include "esp_partition.h"
#include "app_update/include/esp_ota_ops.h"

struct xsFlashIteratorRecord {
	esp_partition_iterator_t	it;
};
typedef struct xsFlashIteratorRecord xsFlashIteratorRecord;
typedef struct xsFlashIteratorRecord *xsFlashIterator;


struct xsFlashRecord {
	const esp_partition_t *partition;
	uint8_t	readOnly;
};
typedef struct xsFlashRecord xsFlashRecord;
typedef struct xsFlashRecord *xsFlash;

#define getPartition(slot) ((xsFlash)xsmcGetHostChunkValidate(slot, xs_flashstorage_destructor))->partition

/*
	Flash Iterator
*/

void xs_flashIterator_(xsMachine *the)
{
	xsFlashIteratorRecord fi;
	
	fi.it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, C_NULL);
	if (!fi.it)
		xsUnknownError("failed");

	xsmcSetHostChunk(xsThis, &fi, sizeof(fi));
}

void xs_flashIterator_destructor(void *data)
{
	xsFlashIterator fi = data;
	if (fi && fi->it)
		esp_partition_iterator_release(fi->it);
}

void xs_flashIterator_next(xsMachine *the)
{
	xsFlashIterator fi = xsmcGetHostChunkValidate(xsThis, xs_flashIterator_destructor);

	xsmcVars(2);
	if (fi->it) {
		const esp_partition_t *partition = esp_partition_get(fi->it);
		fi->it = esp_partition_next(fi->it);

		if (!partition)
			xsUnknownError("failed");

		xsmcSetFalse(xsVar(0));
		xsmcSetString(xsVar(1), (char *)partition->label);
	}
	else {
		xsmcSetTrue(xsVar(0));
	}

	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_flashIterator_return(xsMachine *the)
{
	xsFlashIterator fi = xsmcGetHostChunkValidate(xsThis, xs_flashIterator_destructor);
	if (fi->it) {
		xs_flashIterator_destructor(fi);
		fi->it = C_NULL;
		xs_flashIterator_next(the);
		xsmcSetHostChunk(xsThis, C_NULL, 0);
	}
}

/*
	Flash
*/

void xs_flashstorage_destructor(void *data)
{
}

void xs_flashstorage_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) 
		return;

	xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);

	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_flashstorage_open(xsMachine *the)
{
	if (!xsmcHas(xsArg(0), xsID_path))
		xsUnknownError("no path");

	xsmcGet(xsResult, xsArg(0), xsID_path);
	char *partitionName = xsmcToString(xsResult);
	xsFlashRecord flash = {0};
	if (0 == c_strcmp(partitionName, "xs"))
		flash.partition = esp_partition_find_first(0x40, 1,  NULL);
	else if (0 == c_strcmp(partitionName, "running"))
		flash.partition = esp_ota_get_running_partition();
	else if (0 == c_strcmp(partitionName, "nextota"))
		flash.partition = esp_ota_get_next_update_partition(NULL);
	else
		flash.partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partitionName);

	if (!flash.partition)
		xsUnknownError("can't find partition");

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsResult, xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsResult);
		if (0 == c_strcmp(modeStr, "r"))
			flash.readOnly = 1;
		else if (0 == c_strcmp(modeStr, "r+"))
			;
		else
			xsUnknownError("invalid mode");		//@@ range error
	}

	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("unsupported");

	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, (void *)&flash, sizeof(flash));
}

void xs_flashstorage_eraseBlock(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only");
	const esp_partition_t *partition = flash->partition;
	int start = xsmcToInteger(xsArg(0));
	int stop = start + 1;
	if (xsmcArgc > 1)
		stop = xsmcToInteger(xsArg(1));

	if (ESP_OK != esp_partition_erase_range(partition, start * partition->erase_size, (stop - start) * partition->erase_size))
		xsUnknownError("erase failed");
}

void xs_flashstorage_read(xsMachine *the)
{
	const esp_partition_t *partition = getPartition(xsThis);
	void *buffer;
	xsUnsignedValue byteLength;
	int position = xsmcToInteger(xsArg(1));

	int type = xsmcTypeOf(xsArg(0));
	if ((xsIntegerType == type) || (xsNumberType == type)) {
 		byteLength = xsmcToInteger(xsArg(0));
		xsmcSetArrayBuffer(xsResult, NULL, byteLength);
		buffer = xsmcToArrayBuffer(xsResult);
	}
	else {
		xsmcGetBufferWritable(xsArg(0), &buffer, &byteLength);
		xsmcSetInteger(xsResult, byteLength);
	}

	if (ESP_OK != esp_partition_read(partition, position, buffer, byteLength))
		xsUnknownError("read failed");
}

void xs_flashstorage_write(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only");
	const esp_partition_t *partition = flash->partition;
	int position = xsmcToInteger(xsArg(1));

	void *buffer;
	xsUnsignedValue bufferLength;
	xsmcGetBufferReadable(xsArg(0), &buffer, &bufferLength);

	if (ESP_OK != esp_partition_write(partition, position, buffer, bufferLength))
		xsUnknownError("write failed");
}

void xs_flashstorage_status(xsMachine *the)
{
	const esp_partition_t *partition = getPartition(xsThis);

	xsmcSetNewObject(xsResult);
	
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), partition->size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), partition->erase_size);
	xsmcSet(xsResult, xsID_blockLength, xsVar(0));

	xsmcSetInteger(xsVar(0), partition->size / partition->erase_size);
	xsmcSet(xsResult, xsID_blocks, xsVar(0));
}

const esp_partition_t *getESP32FlashPartition(xsMachine *the, xsSlot *from, uint8_t *writable)
{
	xsFlash flash = xsmcGetHostChunkValidate(*from, xs_flashstorage_destructor);
	*writable = !flash->readOnly;
	return flash->partition;
}
