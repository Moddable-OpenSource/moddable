/*
 * Copyright (c) 2019-2022  Moddable Tech, Inc.
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
		if ((15 != windowBits) && (-15 != windowBits))
			xsRangeError("invalid windowBits");
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

void xs_inflate_close(xsMachine *the)
{
	z_stream *zlib = xsmcGetHostData(xsThis);
	if (zlib && xsmcGetHostDataValidate(xsThis, xs_inflate_destructor)) {
		xs_inflate_destructor(zlib);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_inflate_push(xsMachine *the)
{
	z_stream *zlib = xsmcGetHostDataValidate(xsThis, xs_inflate_destructor);
	uint8_t output[1024];
	int inputOffset = 0;
	uint8_t *input;
	xsUnsignedValue inputRemaining, ignore;
//@@int inputEnd = xsmcTest(xsArg(1));
	int inputEnd = 0;
	int status = Z_OK;

	xsmcGetBufferReadable(xsArg(0), (void **)&input, &inputRemaining);
	while (Z_OK == status) {
		zlib->next_out	= output;
		zlib->avail_out	= sizeof(output);
		zlib->total_out	= 0;

		xsmcGetBufferReadable(xsArg(0), (void **)&input, &ignore);
		zlib->next_in = inputOffset + input;
		zlib->avail_in = inputRemaining;
		zlib->total_in = 0;

		status = inflate(zlib, (inputEnd ? Z_FINISH : MZ_NO_FLUSH));
		if ((Z_OK != status) && (Z_STREAM_END != status)) {
			if (Z_DATA_ERROR == status) {
				xs_inflate_close(the);
				xsUnknownError("bad zlib data");
			}
		}

		if (zlib->total_out) {
			xsmcSetArrayBuffer(xsResult, output, zlib->total_out);
			xsCall1(xsThis, xsID_onData, xsResult);
		}

		inputOffset += zlib->total_in;
		inputRemaining -= zlib->total_in;

		if (Z_STREAM_END == status)
			break;

		if (Z_BUF_ERROR == status) {
			if (!inputRemaining && !zlib->total_out)
				break;
			status = Z_OK;
		}
	}

	if (inputEnd || (status == Z_STREAM_END)) {
		if (Z_STREAM_END != status)
			inflateEnd(zlib);

		xs_inflate_close(the);

		xsmcSetInteger(xsResult, (Z_STREAM_END == status) ? Z_OK : status);
		xsCall1(xsThis, xsID_onEnd, xsResult);
	}

	xsmcSetInteger(xsResult, inputRemaining);
}
