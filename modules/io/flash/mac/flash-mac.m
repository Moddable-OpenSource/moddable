#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "builtinCommon.h"

#import <Cocoa/Cocoa.h>

struct xsFlashRecord {
	xsIntegerValue size;
	xsIntegerValue blocks;
	xsIntegerValue blockLength;
	xsBooleanValue readOnly;
	int fd;
	uint8_t* bytes;
};
typedef struct xsFlashRecord xsFlashRecord;
typedef struct xsFlashRecord *xsFlash;

void xs_flash_partition_destructor(void *it)
{
	if (it) {
		xsFlash flash = it;
		if (flash->bytes != MAP_FAILED)
			munmap(flash->bytes, flash->size);
		if (flash->fd != -1)
			close(flash->fd);
		c_free(flash);
	}
}

void xs_flash_partition_close(xsMachine *the)
{
	xsFlash flash = xsmcGetHostData(xsThis);
	if (flash && xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xs_flash_partition_destructor(flash);
	}
}

void xs_flash_partition_eraseBlock(xsMachine *the)
{
	xsFlash flash = xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only");
	xsIntegerValue start = xsmcToInteger(xsArg(0));
	xsIntegerValue stop = start + 1;
	if (xsmcArgc > 1)
		stop = xsmcToInteger(xsArg(1));
	if ((start < 0) || (flash->blocks <= start))
		xsRangeError("invalid start");
	if ((stop <= 0) || (flash->blocks < stop))
		xsRangeError("invalid stop");
	if (start >= stop)
		xsRangeError("invalid range");
	start *= flash->blockLength;
	stop *= flash->blockLength;
	memset(flash->bytes + start, -1, stop - start);
}

void xs_flash_partition_read(xsMachine *the)
{
	xsFlash flash = xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor);
	void *buffer;
	xsUnsignedValue bufferLength;
	int position = xsmcToInteger(xsArg(1));
	int type = xsmcTypeOf(xsArg(0));
	if ((xsIntegerType == type) || (xsNumberType == type)) {
 		bufferLength = xsmcToInteger(xsArg(0));
		xsmcSetArrayBuffer(xsResult, NULL, bufferLength);
		buffer = xsmcToArrayBuffer(xsResult);
	}
	else {
		xsmcGetBufferWritable(xsArg(0), &buffer, &bufferLength);
		xsmcSetInteger(xsResult, bufferLength);
	}
	memcpy(buffer, flash->bytes + position, bufferLength);
}

void xs_flash_partition_status(xsMachine *the)
{
	xsFlash flash = xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor);
	xsmcVars(1);
	xsmcSetNewObject(xsResult);
	xsmcSetInteger(xsVar(0), flash->size);
	xsmcSet(xsResult, xsID_size, xsVar(0));
	xsmcSetInteger(xsVar(0), flash->blocks);
	xsmcSet(xsResult, xsID_blocks, xsVar(0));
	xsmcSetInteger(xsVar(0), flash->blockLength);
	xsmcSet(xsResult, xsID_blockLength, xsVar(0));
}

void xs_flash_partition_write(xsMachine *the)
{
	xsFlash flash = xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only");
	int position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue bufferLength;
	xsmcGetBufferReadable(xsArg(0), &buffer, &bufferLength);
	uint8_t *p = buffer;
	uint8_t *q = p + bufferLength;
	uint8_t *r = flash->bytes + position;
	while (p < q) {
		*r &= *p;
		p++;
		r++;
	}
}

void xs_flash_partition_get_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor);
	builtinGetFormat(the, kIOFormatBuffer);
}

void xs_flash_partition_set_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, xs_flash_partition_destructor);
	uint8_t format = builtinSetFormat(the);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
}

void xs_flash_partition_open(xsMachine *the)
{
	char path[PATH_MAX];
	xsIndex id;
	xsIntegerValue blocks;
	xsIntegerValue blockLength;
	xsBooleanValue	readOnly = 0;
	xsFlash flash;
	xsmcVars(2);
	
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");
	if (!xsmcHas(xsArg(0), xsID_path))
		xsUnknownError("no path");
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	id = xsToID(xsVar(0));
	if (!xsmcHas(xsArg(2), id))
		xsUnknownError("partition not found");
		
	NSFileManager* manager = [NSFileManager defaultManager];
	NSURL* url = [manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
	url = [url URLByAppendingPathComponent:@PIU_DOT_SIGNATURE isDirectory:YES];
	[manager createDirectoryAtURL:url withIntermediateDirectories:NO attributes:nil error:nil];
	strcpy(path, [url fileSystemRepresentation]);
	strcat(path, "/");
	strcat(path, xsmcToString(xsVar(0)));
	strcat(path, ".flash");
	
	xsmcGet(xsVar(1), xsArg(2), id);
	if (!xsmcHas(xsVar(1), xsID_blocks))
		xsUnknownError("invalid partition");
	xsmcGet(xsVar(0), xsVar(1), xsID_blocks);
	blocks = xsmcToInteger(xsVar(0));
	if (!xsmcHas(xsVar(1), xsID_blockLength))
		xsUnknownError("invalid partition");
	xsmcGet(xsVar(0), xsVar(1), xsID_blockLength);
	blockLength = xsmcToInteger(xsVar(0));
	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsResult, xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsResult);
		if (0 == c_strcmp(modeStr, "r"))
			readOnly = 1;
		else if (0 == c_strcmp(modeStr, "r+"))
			;
		else
			xsUnknownError("invalid mode");
	}
	flash = c_calloc(1, sizeof(xsFlashRecord));
	if (!flash)
		xsRangeError("not enough memory");
	flash->blocks = blocks;
	flash->blockLength = blockLength;
	flash->size = blocks * blockLength;
	flash->readOnly = readOnly;
	flash->fd = -1;
	flash->bytes = MAP_FAILED;
	
	struct stat a_stat;
	if (stat(path, &a_stat) == 0)
		flash->fd = open(path, O_RDWR);
	else
		flash->fd = open(path, O_RDWR | O_CREAT, 0777);
	if (flash->fd == -1)
		xsUnknownError(strerror(errno));
	if (ftruncate(flash->fd, flash->size) == -1)
		xsUnknownError(strerror(errno));
	flash->bytes = mmap(NULL, flash->size, PROT_READ|PROT_WRITE, MAP_SHARED, flash->fd, 0);
	if (flash->bytes == MAP_FAILED)
		xsUnknownError(strerror(errno));
	
	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostData(xsResult, flash);
}
