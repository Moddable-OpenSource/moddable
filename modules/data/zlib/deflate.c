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

void xs_deflate_destructor(void *data)
{
	if (data) {
		deflateEnd((z_stream *)data);
		c_free(data);
	}
}

void xs_deflate(xsMachine *the)
{
	int windowBits = Z_DEFAULT_WINDOW_BITS, level = MZ_DEFAULT_COMPRESSION, strategy = Z_DEFAULT_STRATEGY;
	z_stream *zlib;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_windowBits)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_windowBits);
		windowBits = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_level)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_level);
		level = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_strategy)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_strategy);
		strategy = xsmcToInteger(xsVar(0));
	}

	zlib = c_malloc(sizeof(z_stream));
	if (!zlib)
		xsUnknownError("no memory");

	zlib->zalloc = zAlloc;
	zlib->zfree = zFree;
	
	if (Z_OK != deflateInit2(zlib, level, MZ_DEFLATED, windowBits, 1, strategy)) {
		c_free(zlib);
		xsUnknownError("deflateInit2 failed");
	}

	xsmcSetHostData(xsThis, zlib);
}

void xs_deflate_close(xsMachine *the)
{
	z_stream *zlib = xsmcGetHostData(xsThis);
	xs_deflate_destructor(zlib);
	xsmcSetHostData(xsThis, NULL);
}

void xs_deflate_push(xsMachine *the)
{
	z_stream *zlib = xsmcGetHostData(xsThis);
	uint8_t output[1024];
	int inputOffset = 0;
	int inputRemaining;
	int inputEnd = xsmcTest(xsArg(1));
	int status = Z_OK;
	uint8_t isString = xsStringType == xsmcTypeOf(xsArg(0));

	if (isString)
		inputRemaining = c_strlen(xsmcToString(xsArg(0)));
	else
		inputRemaining = xsmcGetArrayBufferLength(xsArg(0));

	while (0 != inputRemaining) {
		zlib->next_out	= output;
		zlib->avail_out	= sizeof(output);
		zlib->total_out	= 0;

		if (isString)
			zlib->next_in = inputOffset + (uint8_t *)xsmcToString(xsArg(0));
		else
			zlib->next_in = inputOffset + (uint8_t *)xsmcToArrayBuffer(xsArg(0));
		zlib->avail_in = inputRemaining;
		zlib->total_in = 0;

		status = deflate(zlib, (inputEnd ? Z_FINISH : Z_NO_FLUSH));
		if ((Z_OK != status) && (Z_STREAM_END != status)) {
			xs_deflate_close(the);

			xsmcSetInteger(xsResult, status);
			xsCall1(xsThis, xsID_onEnd, xsResult);
			goto bail;
		}

		if (zlib->total_out) {
			xsmcSetArrayBuffer(xsResult, output, zlib->total_out);
			xsCall1(xsThis, xsID_onData, xsResult);
		}

		inputOffset += zlib->total_in;
		inputRemaining -= zlib->total_in;
	}

	if (inputEnd || (status == Z_STREAM_END)) {
		xs_deflate_close(the);

		xsmcSetInteger(xsResult, (Z_STREAM_END == status) ? Z_OK : status);
		xsCall1(xsThis, xsID_onEnd, xsResult);
	}

bail:
	xsmcSetBoolean(xsResult, (Z_OK == status) || (Z_STREAM_END == status));
}
