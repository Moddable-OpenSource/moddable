#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

static void *zAlloc(void *opaque, size_t items, size_t size)
{
	return c_malloc(items * size);
}

static void zFree(void *opaque, void *address)
{
	if (address)
		c_free(address);
}

void xs_inflate_destructor(void *data)
{
	if (data) {
		inflateEnd((z_stream *)data);
		c_free(data);
	}
}

void xs_inflate(xsMachine *the)
{
	int windowBits = 15;
	z_stream *zlib;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_windowBits)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_windowBits);
		windowBits = xsmcToInteger(xsVar(0));
	}

	zlib = c_malloc(sizeof(z_stream));
	if (!zlib)
		xsUnknownError("no memory");

	zlib->zalloc = zAlloc;
	zlib->zfree = zFree;
	
	if (Z_OK != inflateInit2(zlib, windowBits)) {
		c_free(zlib);
		xsUnknownError("inflateInit2 failed");
	}

	xsmcSetHostData(xsThis, zlib);
}

void xs_inflate_push(xsMachine *the)
{
	z_stream *zlib = xsmcGetHostData(xsThis);
	uint8_t output[1024];
	int inputOffset = 0;
	int inputRemaining = xsmcGetArrayBufferLength(xsArg(0));
	int inputEnd = xsmcTest(xsArg(1));
	int status = 0;

	while (0 == status) {
		zlib->next_out	= output;
		zlib->avail_out	= sizeof(output);
		zlib->total_out	= 0;

		zlib->next_in = inputOffset + (uint8_t *)xsmcToArrayBuffer(xsArg(0));
		zlib->avail_in = inputRemaining;
		zlib->total_in = 0;

		status = inflate(zlib, Z_PARTIAL_FLUSH);
		if ((Z_OK != status) && (Z_STREAM_END != status)) {
			if (Z_DATA_ERROR == status)
				xsUnknownError("bad zlib data");
		}
		if (status == Z_BUF_ERROR)
			status = Z_OK;

		if (zlib->total_out) {
			xsmcSetArrayBuffer(xsResult, output, zlib->total_out);
			xsCall1(xsThis, xsID_onData, xsResult);
		}

		inputOffset += zlib->total_in;
		inputRemaining -= zlib->total_in;
	}

	if (inputEnd || (status == Z_STREAM_END)) {
		if (Z_STREAM_END != status)
			inflateEnd(zlib);

		xsmcSetInteger(xsResult, (Z_STREAM_END == status) ? Z_OK : status);
		xsCall1(xsThis, xsID_onEnd, xsResult);
	}

	xsmcSetUndefined(xsResult);
}
