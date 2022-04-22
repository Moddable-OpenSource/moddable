/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"			// esp platform support

#include "modSPI.h"

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"

extern void modDigitalBankWrite(void *bank, uint32_t value);
extern xsHostHooks xsDigitalBankHooks;

static void xs_display_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsDisplayHooks = {
	xs_display_destructor,
	xs_display_mark,
	NULL
};

static void displayBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void displaySend(PocoPixel *pixels, int byteCount, void *refCon);
static void displayEnd(void *refcon);

static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	displayBegin,
	displayEnd,		// continue
	displayEnd,
	displaySend,
	NULL			// adapt invalid
};

enum {
	kDisplayFlagDrawing = 1 << 0,
	kDisplayFlagHaveMAC = 1 << 1,
	kDisplayFlagHaveInvert = 1 << 2,
	kDisplayFlagFirstFrame = 1 << 3,
	kDisplayFlagTransformPixels = 1 << 4,
	kDisplayFlagDCIsBank = 1 << 5,
	kDisplayFlagAsync = 1 << 6
};

struct DisplayRecord {
	PixelsOutDispatch			dispatch;	// must be first!

//@@ validator / magic?

	xsMachine					*the;
	modSPIConfigurationRecord	*spi;
	union {
		xsSlot					*obj;
		void					*bank;
	} dc;
	xsSlot						*state;
	xsSlot						*async;
	uint8_t						flags;
	uint8_t						rotation;
	CommodettoDimension			x;
	CommodettoDimension			y;
	CommodettoDimension			width;
	CommodettoDimension			height;
};
typedef struct DisplayRecord DisplayRecord;
typedef struct DisplayRecord *Display;

static void displayCommand(Display disp, uint8_t command, const uint8_t *data, uint16_t count);
static void displaySetDC(Display disp, xsIntegerValue dc);

void xs_display_destructor(void *data)
{
	if (!data)
		return;

	c_free(data);
}

void xs_display_initialize(xsMachine* the)
{
	if (xsmcArgc) {
		Display disp = c_calloc(1, sizeof(DisplayRecord));
		if (!disp)
			xsUnknownError("no memory");
		xsmcSetHostData(xsThis, disp);
		xsSetHostHooks(xsThis, (xsHostHooks *)&xsDisplayHooks);

		disp->spi = xsmcGetHostDataValidate(xsArg(0), xs_spi_destructor);
		disp->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;
		disp->state = xsToReference(xsArg(2));
		disp->the = the;
		disp->flags = kDisplayFlagFirstFrame;

		if (&xsDigitalBankHooks == xsGetHostHooks(xsArg(1))) {
			disp->dc.bank = xsmcGetHostData(xsArg(1));
			disp->flags |= kDisplayFlagDCIsBank; 
		}
		else
			disp->dc.obj = xsToReference(xsArg(1));
	}
	else {
		Display disp = xsmcGetHostData(xsThis);
		if (disp && xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks)) {
			xs_display_destructor(disp);
			xsmcSetHostData(xsThis, NULL);
			xsmcSetHostDestructor(xsThis, NULL);
		}
	}
}

void xs_display_configure(xsMachine *the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsIntegerValue value;

	switch (xsmcToInteger(xsArg(0))) {
		case 0:		// width, height
			disp->width = xsmcToInteger(xsArg(1));
			disp->height = xsmcToInteger(xsArg(2));
			break;
		case 1:
			value = xsmcToInteger(xsArg(1));
			if (kCommodettoBitmapRGB565BE == value)
				disp->flags &= ~kDisplayFlagTransformPixels;
			else if (kCommodettoBitmapRGB565LE == value)
				disp->flags |= kDisplayFlagTransformPixels;
			else
				xsRangeError("invalid");
			break;
		case 2:		// rotation
			disp->rotation = xsmcToInteger(xsArg(1));
			break;
		case 3:		// position (at rotation 0)
			disp->x = xsmcToInteger(xsArg(1));
			disp->y = xsmcToInteger(xsArg(2));
			break;
		case 4:
			if (xsmcTest(xsArg(1)))
				disp->flags |= kDisplayFlagAsync;
			else
				disp->flags &= ~kDisplayFlagAsync;
			break;
	}
}

void xs_display_begin(xsMachine* the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsSlot tmp;
	xsIntegerValue x, y, w, h;
	CommodettoDimension tw, th;

	if (0 == xsmcArgc) {
		if (disp->flags & kDisplayFlagDrawing)
			xsUnknownError("bad state");
		displayBegin(disp, 0, 0, disp->width, disp->height);
		return;
	}

	if (disp->flags & kDisplayFlagDrawing) {
		xsmcGet(tmp, xsArg(0), xsID_continue);
		if (!xsmcTest(tmp))
			xsUnknownError("bad state");
	}

	xsmcGet(tmp, xsArg(0), xsID_x);
	x = xsmcToInteger(tmp);
	xsmcGet(tmp, xsArg(0), xsID_y);
	y = xsmcToInteger(tmp);
	xsmcGet(tmp, xsArg(0), xsID_width);
	w = xsmcToInteger(tmp);
	xsmcGet(tmp, xsArg(0), xsID_height);
	h = xsmcToInteger(tmp);

	tw = (disp->rotation & 1) ? disp->height : disp->width; 
	th = (disp->rotation & 1) ? disp->width : disp->height; 
	if ((w <= 0) || (h <= 0) || (x < 0) || (y < 0) || (x > tw) || (y > th) || (w > tw) || (h > th))
		xsRangeError("bad coordinates");

	displayBegin(disp, (CommodettoCoordinate)x, (CommodettoCoordinate)y, (CommodettoDimension)w, (CommodettoDimension)h);

	disp->async = NULL;
}

void xs_display_send(xsMachine* the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	void *data;
	xsUnsignedValue count;
	uint8_t nonrelocatable = xsBufferNonrelocatable == xsmcGetBufferReadable(xsArg(0), &data, &count);
	if (count > 65535)
		xsRangeError("unsupported byteLength");

	if (nonrelocatable && (kDisplayFlagAsync & disp->flags)) {
		count = (xsUnsignedValue)(-(int)count);
		disp->async = xsToReference(xsArg(0));
	}
	else
		disp->async = NULL;

	displaySend((PocoPixel *)data, count, disp);
}

void xs_display_end(xsMachine* the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	displayEnd(disp);
}

void xs_display_adaptInvalid(xsMachine* the)
{
	//@@ nothing for most pixel formats
}

void xs_display_command(xsMachine *the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsIntegerValue command;
	uint8_t data[32];
	int argc = xsmcArgc, i;
	xsSlot tv, tr;

	if ((disp->flags & kDisplayFlagDrawing) || (argc > (sizeof(data) - 1)))
		xsUnknownError("bad state");

	command = xsmcToInteger(xsArg(0));

	for (i = 1; i < argc; i++)
		data[i - 1] = xsmcToInteger(xsArg(i));

	if ((0x36 == command) && !(disp->flags & kDisplayFlagHaveMAC)) {
		disp->flags |= kDisplayFlagHaveMAC;
		xsmcSetInteger(tv, data[0]);
		tr = xsReference(disp->state);
		xsmcSet(tr, xsID_mac, tv);
	}
	else if ((0x20 == (command & 0xFE)) && !(disp->flags & kDisplayFlagHaveInvert)) {
		disp->flags |= kDisplayFlagHaveInvert;
		xsmcSetInteger(tv, command & 1);
		tr = xsReference(disp->state);
		xsmcSet(tr, xsID_invert, tv);
	}

	displayCommand(disp, command, data, argc - 1);
}

void xs_display_get_width(xsMachine *the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsmcSetInteger(xsResult, (disp->rotation & 1) ? disp->height : disp->width);
}

void xs_display_get_height(xsMachine *the)
{
	Display disp = xsmcGetHostDataValidate(xsThis, (void *)&xsDisplayHooks);
	xsmcSetInteger(xsResult, (disp->rotation & 1) ? disp->width : disp->height);
}

void displayBegin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	Display disp = refcon;
	uint8_t data[4];
	uint16_t xMin, xMax, yMin, yMax;

	if (disp->flags & kDisplayFlagDrawing)
		return;

//@@ apply rotation to keep physical area consistent
	xMin = x + disp->x;
	yMin = y + disp->y;

	xMax = xMin + w - 1;
	yMax = yMin + h - 1;

	data[0] = xMin >> 8;
	data[1] = xMin & 0xff;
	data[2] = xMax >> 8;
	data[3] = xMax & 0xff;
	displayCommand(disp, 0x2a, data, 4);

	data[0] = yMin >> 8;
	data[1] = yMin & 0xff;
	data[2] = yMax >> 8;
	data[3] = yMax & 0xff;
	displayCommand(disp, 0x2b, data, 4);

    displayCommand(disp, 0x2c, NULL, 0);

	displaySetDC(disp, 1);
}

void displaySend(PocoPixel *pixels, int byteCount, void *refCon)
{
	Display disp = refCon;

	if (byteCount < 0) {
		byteCount = -byteCount;
		modSPISetSync(disp->spi, (kDisplayFlagAsync & disp->flags) ? false : true);
	}
	else
		modSPISetSync(disp->spi, true);

	if (disp->flags & kDisplayFlagTransformPixels) {
#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
		modSPITxSwap16(disp->spi, (uint8_t *)pixels, (uint16_t)byteCount);
#else
		//@@ other pixel formats
#endif
	}
	else
		modSPITx(disp->spi, (uint8_t *)pixels, (uint16_t)byteCount);
}

void displayEnd(void *refCon)
{
	Display disp = refCon;

	if (disp->flags & kDisplayFlagFirstFrame)
		displayCommand(disp, 0x29, NULL, 0);
	
	disp->flags &= ~(kDisplayFlagDrawing | kDisplayFlagFirstFrame);
}

void displaySetDC(Display disp, xsIntegerValue dc)
{
	if (disp->flags & kDisplayFlagDCIsBank)
		modDigitalBankWrite(disp->dc.bank, dc ? ~0 : 0);
	else {
		xsMachine *the = disp->the;
		xsCall1(xsReference(disp->dc.obj), xsID_write, xsInteger(dc));
	}
}

void displayCommand(Display disp, uint8_t command, const uint8_t *data, uint16_t count)
{
	xsMachine *the = disp->the;

	modSPIActivateConfiguration(NULL);
	displaySetDC(disp, 0);
   	modSPITxRx(disp->spi, &command, 1);		// use modSPITxRx to be synchronous

	if (count) {
		displaySetDC(disp, 1);
        modSPITxRx(disp->spi, (uint8_t *)data, count);
    }
}

void xs_display_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Display disp = it;

	if (disp->async)
		(*markRoot)(the, disp->async);
}
