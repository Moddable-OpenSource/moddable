/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"

#include "commodettoPocoBlit.h"

#include "esp_jpeg_dec.h"

static void xs_jpeg_buffer_destructor(void *data);
static void xs_JPEG_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsJPEGHooks = {
	xs_JPEG_destructor,
	xs_JPEG_mark,
	NULL
};

typedef struct {
	jpeg_dec_handle_t		handle;
	jpeg_dec_header_info_t 	info;
	void					*output;
	int						outputLength;
	int						y;
	int						slabHeight;
	xsSlot					*input;
	int						inbuf_remain;
	xsSlot					*bitmap;
} JPEGRecord, *JPEG;

void xs_jpeg_buffer_destructor(void *data)
{
	if (data)
		jpeg_free_align(data);
}

void xs_jpeg_buffer_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	xs_jpeg_buffer_destructor(data);
	xsmcSetHostBuffer(xsThis, NULL, 0);
}

void xs_JPEG_destructor(void *data)
{
	JPEG jpeg = data;
	if (jpeg && jpeg->handle)
		jpeg_dec_close(jpeg->handle);
}

#define xsConstruct5(_FUNCTION, _SLOT0, _SLOT1, _SLOT2, _SLOT3, _SLOT4) \
	(xsOverflow(-XS_FRAME_COUNT-5), \
	fxPush(_FUNCTION), \
	fxNew(the), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxRunCount(the, 5), \
	fxPop())

void xs_JPEG_initialize(xsMachine *the)
{
	JPEGRecord jpeg;
	xsSlot tmp;
	void *data;
	xsUnsignedValue dataLength;
	uint8_t all = 0;

	xsmcGetBufferReadable(xsArg(0), &data, &dataLength);		// confirm that thiw is a buffer

	if (xsmcTest(xsArg(1))) { 
		xsmcGet(tmp, xsArg(1), xsID_all);		// undefined if property missing
		all = xsmcTest(tmp);
	}

	jpeg_dec_config_t config = {
		.output_type  = JPEG_PIXEL_FORMAT_RGB565_LE,
		.scale        = {.width = 0, .height = 0},
		.clipper      = {.width = 0, .height = 0},
		.rotate       = JPEG_ROTATE_0D,
		.block_enable = !all,
	};

	if (ESP_OK != jpeg_dec_open(&config, &jpeg.handle))
		xsUnknownError("open failed");

	jpeg_dec_io_t io = {
		.inbuf         = data,
		.inbuf_len     = dataLength,
	};

	if (ESP_OK != jpeg_dec_parse_header(jpeg.handle, &io, &jpeg.info)) {
		jpeg_dec_close(jpeg.handle);
		xsUnknownError("parse failed");
	}
	jpeg.inbuf_remain = io.inbuf_remain;

	int outbuf_len;
	if (ESP_OK != jpeg_dec_get_outbuf_len(jpeg.handle, &jpeg.outputLength)) {
		jpeg_dec_close(jpeg.handle);
		xsUnknownError("get output len failed");
	}

	jpeg.output = jpeg_calloc_align(jpeg.outputLength, 16);
	if (NULL == jpeg.output) {
		jpeg_dec_close(jpeg.handle);
		xsUnknownError("no memory");
	}

	xsmcSetInteger(tmp, jpeg.info.width);
	xsmcDefine(xsThis, xsID_width, tmp, xsDontDelete | xsDontSet);
	xsmcSetInteger(tmp, jpeg.info.height);
	xsmcDefine(xsThis, xsID_height, tmp, xsDontDelete | xsDontSet);
	xsmcSetInteger(tmp, kCommodettoBitmapRGB565LE);
	xsmcDefine(xsThis, xsID_pixelFormat, tmp, xsDontDelete | xsDontSet);

	jpeg.input = xsmcToReference(xsArg(0));

	xsmcVars(2);
	xsVar(0) = xsNewHostObject(xs_jpeg_buffer_destructor);
	xsmcSetHostBuffer(xsVar(0), jpeg.output, jpeg.outputLength);
	xsVar(1) = xsNewHostFunction(xs_jpeg_buffer_close, 0);
	xsmcDefine(xsVar(0), xsID_close, xsVar(1), xsDontDelete | xsDontSet);
	xsmcSetInteger(tmp, jpeg.outputLength);
	xsmcDefine(xsVar(0), xsID_byteLength, tmp, xsDontDelete | xsDontSet);

	jpeg.y = 0;
	jpeg.slabHeight = (jpeg.outputLength / 2) / jpeg.info.width;		// assumes JPEG_PIXEL_FORMAT_RGB565_*
	xsmcSetHostChunk(xsThis, &jpeg, sizeof(jpeg));
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsJPEGHooks);

	xsVar(0) = xsConstruct5(xsArg(2), xsInteger(0), xsInteger(0), xsInteger(kCommodettoBitmapRGB565LE), xsVar(0), xsInteger(0));
	JPEG j = xsmcGetHostChunkValidate(xsThis, (void *)&xsJPEGHooks);
	j->bitmap = xsmcToReference(xsVar(0));
	xsmcSetInteger(tmp, 0);
	xsmcSet(xsVar(0), xsID_x, tmp);

	CommodettoBitmap cb = xsmcGetHostChunk(xsVar(0));
	cb->w = jpeg.info.width;
	cb->h = jpeg.slabHeight;
}

void xs_JPEG_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) return;

	JPEG jpeg = xsmcGetHostChunkValidate(xsThis, (void *)&xsJPEGHooks);
	xs_JPEG_destructor(jpeg);
	xsmcSetHostChunk(xsThis, NULL, 0);
	xsSetHostDestructor(xsThis, NULL);
}

void xs_JPEG_read(xsMachine *the)
{
	JPEG jpeg = xsmcGetHostChunkValidate(xsThis, (void *)&xsJPEGHooks);
	void *data;
	xsUnsignedValue dataLength;

	if (!jpeg->handle) return;		// end of image

	xsmcVars(2);

	xsVar(0) = xsReference(jpeg->input);
	xsmcGetBufferReadable(xsVar(0), (void **)&data, &dataLength);

	jpeg_dec_io_t io = {
		.inbuf         = data,
		.inbuf_len     = dataLength,
		.inbuf_remain  = jpeg->inbuf_remain,
		.outbuf        = jpeg->output,
		.out_size      = jpeg->outputLength,
	};

	if (ESP_OK != jpeg_dec_process(jpeg->handle, &io))
		xsUnknownError("process failed");
	jpeg->inbuf_remain = io.inbuf_remain;

	xsResult = xsReference(jpeg->bitmap);
	xsSlot tmp;
	xsmcSetInteger(tmp, jpeg->y);
	xsmcSet(xsResult, xsID_y, tmp);

	jpeg = xsmcGetHostChunkValidate(xsThis, (void *)&xsJPEGHooks);

	jpeg->y += jpeg->slabHeight;
	if (jpeg->y >= jpeg->info.height) {
		jpeg_dec_close(jpeg->handle);
		jpeg->handle = NULL;
		jpeg->input = NULL;
		jpeg->bitmap = NULL;
	}
}

void xs_JPEG_get_ready(xsMachine *the)
{
	JPEG jpeg = xsmcGetHostChunkValidate(xsThis, (void *)&xsJPEGHooks);
	xsmcSetBoolean(xsResult, jpeg->handle ? true : false);
}

static void xs_JPEG_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	JPEG jpeg = it;

	if (jpeg->input)
		(*markRoot)(the, jpeg->input);

	if (jpeg->bitmap)
		(*markRoot)(the, jpeg->bitmap);
}
