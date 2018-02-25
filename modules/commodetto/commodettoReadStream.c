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
	uint32_t	offset;
	uint8_t		loop;
	uint16_t	nextFrameSize;
} CSRecord, *CS;

static void doRead(xsMachine *the, CS cs, void *data, uint16_t size);

void xs_cs_destructor(void *data)
{
}

void xs_cs_constructor(xsMachine *the)
{
	CS cs;
	ColorCellHeaderRecord cch;
	uint16_t nextFrameSize;

	xsmcVars(1);

	cs = xsmcSetHostChunk(xsThis, NULL, sizeof(CSRecord));

	if (xsmcArgc < 2)
		xsCall1(xsThis, xsID_initialize, xsArg(0));
	else {
		xsmcGet(xsVar(0), xsArg(1), xsID_loop);
		cs->loop = xsmcTest(xsVar(0));

		xsCall2(xsThis, xsID_initialize, xsArg(0), xsArg(1));
	}

	doRead(the, cs, &cch, sizeof(cch));
	if (('c' != c_read8(&cch.id_c)) || ('s' != c_read8(&cch.id_s)))
		xsErrorPrintf("bad stream");

	if (kCommodettoBitmapRGB565LE != c_read8(&cch.bitmapFormat))
		xsErrorPrintf("bad pixel format");

	if (0 != cch.reserved)
		xsErrorPrintf("bad reserved");

	xsmcSetInteger(xsVar(0), cch.width);
	xsmcSet(xsThis, xsID_width, xsVar(0));
	xsmcSetInteger(xsVar(0), cch.height);
	xsmcSet(xsThis, xsID_height, xsVar(0));
	xsmcSetInteger(xsVar(0), cch.bitmapFormat);
	xsmcSet(xsThis, xsID_pixelFormat, xsVar(0));
	//@@ frame rate too...

	cs = xsmcGetHostChunk(xsThis);
	doRead(the, cs, &nextFrameSize, sizeof(nextFrameSize));
	cs = xsmcGetHostChunk(xsThis);
	cs->nextFrameSize = nextFrameSize;
}

void xs_cs_next(xsMachine *the)
{
	CS cs = xsmcGetHostChunk(xsThis);
	uint16_t nextFrameSize = cs->nextFrameSize;
	uint8_t *data;

	xsmcVars(1);

	if (0 == nextFrameSize) {
		if (!cs->loop)
			return;

		cs->offset = sizeof(ColorCellHeaderRecord);
		doRead(the, cs, &nextFrameSize, sizeof(nextFrameSize));
		cs = xsmcGetHostChunk(xsThis);
		cs->nextFrameSize = nextFrameSize;
	}

	xsResult = xsCall2(xsThis, xsID_read, xsInteger(cs->offset), xsInteger(nextFrameSize + 2));
	cs->offset += nextFrameSize + 2;

	if (xsmcIsInstanceOf(xsResult, xsArrayBufferPrototype))
		data = (uint8_t *)xsmcToArrayBuffer(xsResult);
	else
		data = xsmcGetHostData(xsResult);
	c_memcpy(&nextFrameSize, data + nextFrameSize, sizeof(nextFrameSize));
	cs->nextFrameSize = nextFrameSize;
}

void doRead(xsMachine *the, CS cs, void *data, uint16_t size)
{
	void *src;

	xsVar(0) = xsCall2(xsThis, xsID_read, xsInteger(cs->offset), xsInteger(size));

	if (xsmcIsInstanceOf(xsVar(0), xsArrayBufferPrototype))
		src = (uint8_t *)xsmcToArrayBuffer(xsVar(0));
	else
		src = xsmcGetHostData(xsVar(0));
	c_memcpy(data, src, size);

	cs = xsmcGetHostChunk(xsThis);
	cs->offset += size;
}
