/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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


#include "xsPlatform.h"
#include "xsmc.h"
#include "commodettoBitmap.h"
#include "commodettoStream.h"
#include "mc.xs.h"			// for xsID_ values

typedef struct {
	uint8_t		*data;
	uint32_t	dataSize;

	uint32_t	offset;
	uint8_t		loop;
} CSRecord, *CS;

void xs_cs_destructor(void *data)
{
	CS cs = data;
	if (cs)
		c_free(cs);
}

void xs_cs_constructor(xsMachine *the)
{
	CS cs;
	ColorCellHeader cch;

	cs = c_calloc(1, sizeof(CSRecord));
	if (!cs)
		xsErrorPrintf("no memory");

	xsmcSetHostData(xsThis, cs);

//@@ keep reference to buffer
	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		cs->data = xsmcToArrayBuffer(xsArg(0));
		cs->dataSize = xsGetArrayBufferLength(xsArg(0));
	}
	else {
		xsmcVars(1);
		cs->data = xsmcGetHostData(xsArg(0));
		xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
		cs->dataSize = xsmcToInteger(xsVar(0));
	}

	cch = (ColorCellHeader)cs->data;
	if (('c' != c_read8(&cch->id_c)) || ('s' != c_read8(&cch->id_s)))
		xsErrorPrintf("bad stream");

	if (kCommodettoBitmapRGB565LE != c_read8(&cch->bitmapFormat))
		xsErrorPrintf("bad pixel format");

	if (0 != c_read8(&cch->reserved))
		xsErrorPrintf("bad reserved");

	cs->offset = sizeof(ColorCellHeaderRecord);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), c_read16(&cch->width));
	xsmcSet(xsThis, xsID_width, xsVar(0));
	xsmcSetInteger(xsVar(0), c_read16(&cch->height));
	xsmcSet(xsThis, xsID_height, xsVar(0));
	xsmcSetInteger(xsVar(0), c_read16(&cch->bitmapFormat));
	xsmcSet(xsThis, xsID_pixelFormat, xsVar(0));

	if (xsmcArgc > 1) {
		xsmcGet(xsVar(0), xsArg(1), xsID_loop);
		cs->loop = xsmcTest(xsVar(0));
	}
}

void xs_cs_next(xsMachine *the)
{
	CS cs = xsmcGetHostData(xsThis);
	uint16_t size;
	uint8_t *data;

	if (cs->offset >= cs->dataSize) {
		if (!cs->loop)
			return;
		cs->offset = sizeof(ColorCellHeaderRecord);
	}

	data = cs->data + cs->offset;
	size = c_read16(data);
	if ((cs->offset + size) > cs->dataSize)
		xsErrorPrintf("bad stream");

	xsResult = xsNewHostObject(NULL);
	xsmcSetHostData(xsResult, data + sizeof(uint16_t));
	xsmcVars(1);
	xsVar(0) = xsInteger(size);
	xsmcSet(xsResult, xsID_byteLength, xsVar(0));

	cs->offset += size + sizeof(uint16_t);
}
