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
	uint16_t	nextFrameSize;
} CSRecord, *CS;

void xs_cs_destructor(void *data)
{
}

void xs_cs_constructor(xsMachine *the)
{
	CSRecord cs = {0};
	ColorCellHeaderRecord cch;

	xsmcVars(1);
	cs.data = xsmcGetHostData(xsArg(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_byteLength);
	cs.dataSize = xsmcToInteger(xsVar(0));

	c_memcpy(&cch, cs.data, sizeof(cch));
	if (('c' != c_read8(&cch.id_c)) || ('s' != c_read8(&cch.id_s)))
		xsErrorPrintf("bad stream");

	if (kCommodettoBitmapRGB565LE != c_read8(&cch.bitmapFormat))
		xsErrorPrintf("bad pixel format");

	if (0 != cch.reserved)
		xsErrorPrintf("bad reserved");

	cs.offset = sizeof(ColorCellHeaderRecord);

	xsmcSetInteger(xsVar(0), cch.width);
	xsmcSet(xsThis, xsID_width, xsVar(0));
	xsmcSetInteger(xsVar(0), cch.height);
	xsmcSet(xsThis, xsID_height, xsVar(0));
	xsmcSetInteger(xsVar(0), cch.bitmapFormat);
	xsmcSet(xsThis, xsID_pixelFormat, xsVar(0));

	cs.nextFrameSize = c_read16(cs.data + cs.offset);
	cs.offset += 2;

	if (xsmcArgc > 1) {
		xsmcGet(xsVar(0), xsArg(1), xsID_loop);
		cs.loop = xsmcTest(xsVar(0));
	}

	xsmcSetHostChunk(xsThis, &cs, sizeof(cs));
}

void xs_cs_next(xsMachine *the)
{
	CS cs = xsmcGetHostChunk(xsThis);
	uint8_t *data;
	uint16_t nextFrameSize = cs->nextFrameSize;

	if (cs->offset >= cs->dataSize) {
		if (!cs->loop)
			return;
		cs->offset = sizeof(ColorCellHeaderRecord);

		nextFrameSize = cs->nextFrameSize = c_read16(cs->data + cs->offset);
		cs->offset += 2;
	}

	data = cs->data + cs->offset;
	if ((cs->offset + nextFrameSize) > cs->dataSize)
		xsErrorPrintf("bad stream");

	xsResult = xsNewHostObject(NULL);
	xsmcSetHostData(xsResult, data);
	xsmcVars(1);
	xsVar(0) = xsInteger(nextFrameSize);
	xsmcSet(xsResult, xsID_byteLength, xsVar(0));

	cs = xsmcGetHostChunk(xsThis);
	cs->offset += nextFrameSize;

	if (cs->offset + 2 <= cs->dataSize) {
		cs->nextFrameSize = c_read16(cs->data + cs->offset);
		cs->offset += 2;
	}
	else
		cs->offset = cs->dataSize;
}
