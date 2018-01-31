/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsAll.h"
#include "xs.h"
#include "modInstrumentation.h"
#include "screen.h"
#include "mc.xs.h"

//#include "commodettoBitmapFormat.h"
#define kCommodettoBitmapMonochrome (3)
#define kCommodettoBitmapGray16 (4)
#define kCommodettoBitmapGray256 (5)
#define kCommodettoBitmapRGB332 (6)
#define kCommodettoBitmapRGB565LE (7)
#define kCommodettoBitmapRGB565BE (8)
#define kCommodettoBitmap24RGB (9)
#define kCommodettoBitmap32RGBA (10)
#define kCommodettoBitmapCLUT16 (11)
#define kCommodettoBitmapRGB444 (12)

#define mxScreenIdling 1

static xsBooleanValue fxFindResult(xsMachine* the, xsSlot* slot, xsIndex id);
#define xsFindResult(_THIS,_ID) fxFindResult(the, &_THIS, _ID)
static xsBooleanValue fxArchiveRead(void* src, size_t offset, void* buffer, size_t size);
static xsBooleanValue fxArchiveWrite(void* src, size_t offset, void* buffer, size_t size);

typedef struct {
	xsIntegerValue bytesPerPixel;
	xsIntegerValue bitsPerPixel;
	xsIntegerValue commodettoFormat;
} txPixelFormat;

typedef struct sxWorker txWorker;
struct sxWorker {
	txScreen* screen;
	txWorker* nextWorker;
	xsIntegerValue id;
	xsSlot slot;
};

extern void fxAbortCallback(void *info);
extern txPreparation* xsPreparation();

static void fxScreenIdle(txScreen* screen);
static void fxScreenInvoke(txScreen* screen, char* message, int size);
#if mxMacOSX
extern __attribute__ ((visibility("default"))) void fxScreenLaunch(txScreen* screen);
#elif mxWindows
extern __declspec( dllexport ) void fxScreenLaunch(txScreen* screen);
#else
extern void fxScreenLaunch(txScreen* screen);
#endif
static void fxScreenQuit(txScreen* screen);
static void fxScreenSetPalette(txScreen* screen, uint16_t* clut);
static void fxScreenTouch(txScreen* screen, int kind, int index, int x, int y, double when);

static void screen_adaptInvalid(xsMachine* the);
static void screen_animateColors(xsMachine* the);
static void screen_begin(xsMachine* the);
static void screen_clear(xsMachine* the);
static void screen_continue(xsMachine* the);
static void screen_end(xsMachine* the);
static void screen_get_clut(xsMachine* the);
static void screen_pixelsToBytes(xsMachine* the);
static void screen_postMessage(xsMachine* the);
static void screen_send(xsMachine* the);
static void screen_set_clut(xsMachine* the);
static void screen_start(xsMachine* the);
static void screen_stop(xsMachine* the);
static void screen_get_pixelFormat(xsMachine* the);
static void screen_get_width(xsMachine* the);
static void screen_get_height(xsMachine* the);
static void screen_set_pixelFormat(xsMachine* the);
#if kPocoFrameBuffer
static void screen_get_frameBuffer(xsMachine* the);
#endif

#ifdef mxInstrument
#define screenInstrumentCount kModInstrumentationPiuCommandListUsed - kModInstrumentationPixelsDrawn + 1
static char* screenInstrumentNames[screenInstrumentCount] = {
	"Pixels drawn",
	"Frames drawn",
	"Network bytes read",
	"Network bytes written",
	"Network sockets",
	"Timers",
	"Files",
	"Poco display list used",
	"Piu command List used",
};
static char* screenInstrumentUnits[screenInstrumentCount] = {
	" pixels",
	" frames",
	" bytes",
	" bytes",
	" sockets",
	" timers",
	" files",
	" bytes",
	" bytes",
};
txInteger screenInstrumentValues[screenInstrumentCount];
static void fxScreenSampleInstrumentation(txScreen* screen);
#endif

static txPixelFormat gxPixelFormats[pixelFormatCount] = {
	{ 2, 16, kCommodettoBitmapRGB565LE },
	{ 2, 16, kCommodettoBitmapRGB565BE },
	{ 1, 8, kCommodettoBitmapGray256 },
	{ 1, 8, kCommodettoBitmapRGB332 },
	{ 1, 4, kCommodettoBitmapGray16 },
	{ 1, 4, kCommodettoBitmapCLUT16 },
};

static char* gxTouchEventNames[4] = {
	"onTouchBegan",
	"onTouchCancelled",
	"onTouchEnded",
	"onTouchMoved",
};

void fxAbort(xsMachine* the)
{
	txScreen* screen = the->host;
	(*screen->abort)(screen);
}

void debugBreak(xsMachine* the, uint8_t stop)
{
#ifdef mxInstrument
	if (stop) {
		fxCollectGarbage(the);
		the->garbageCollectionCount -= 1;
		fxScreenSampleInstrumentation((txScreen*)the->host);
	}
#endif
}

void fxScreenIdle(txScreen* screen)
{
	if (screen->machine) {
#ifdef mxInstrument
		c_timeval tv;
		c_gettimeofday(&tv, NULL);
		if (screen->instrumentTime < tv.tv_sec) {
			screen->instrumentTime = tv.tv_sec;
			fxScreenSampleInstrumentation(screen);
		}
#endif	
		if (screen->flags & mxScreenIdling) {
			xsBeginHost(screen->machine);
			{
				xsVars(2);
				xsVar(0) = xsGet(xsGlobal, xsID("screen"));
				xsVar(1) = xsGet(xsVar(0), xsID("context"));
				if (xsTest(xsVar(1))) {
					if (xsFindResult(xsVar(1), xsID("onIdle"))) {
						xsCallFunction0(xsResult, xsVar(1));
					}
				}
			}
			xsEndHost(screen->machine);
		}
	}
}

void fxScreenInvoke(txScreen* screen, char* buffer, int size)
{
	xsBeginHost(screen->machine);
	if (size < 0) {
		int id = 0 - size;
		txWorker* worker = screen->firstWorker;
		while (worker) {
			if (worker->id == id)
				break;
			worker = worker->nextWorker;
		}
		if (worker) {
			xsVars(2);
			xsVar(0) = xsAccess(worker->slot);
			xsVar(1) = xsDemarshallAlien(buffer);
			if (xsFindResult(xsVar(0), xsID("onmessage"))) {
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
			else if (xsFindResult(xsVar(0), xsID("onMessage"))) {
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			}
		}
	}
	else {
		xsVars(2);
		xsVar(0) = xsGet(xsGlobal, xsID("screen"));
		xsVar(1) = xsGet(xsVar(0), xsID("context"));
		if (xsTest(xsVar(1))) {
			if (xsFindResult(xsVar(1), xsID("onMessage"))) {
				if (size)
					(void)xsCallFunction1(xsResult, xsVar(1), xsArrayBuffer(buffer, size));
				else
					(void)xsCallFunction1(xsResult, xsVar(1), xsString(buffer));
	
			}
		}
	}
	xsEndHost(screen->machine);
}

void fxScreenLaunch(txScreen* screen)
{
	txPreparation* preparation = xsPreparation();
	txMachine _root;
	txMachine* root = &_root;
	if (!preparation) {
		return;
	}
	if ((preparation->version[0] != XS_MAJOR_VERSION) || (preparation->version[1] != XS_MINOR_VERSION) || (preparation->version[2] != XS_PATCH_VERSION)) {
		//info = [NSString stringWithFormat:@"Require version %d.%d.%d", XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION];
		return;
	}
	
	root->preparation = preparation;
	if (screen->archive)
		root->archive = fxMapArchive(preparation, screen->archive, screen->archive, 4 * 1024, fxArchiveRead, fxArchiveWrite);
	else
		root->archive = NULL;
	root->keyArray = preparation->keys;
	root->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.keyCount;
	root->keyIndex = (txID)preparation->keyCount;
	root->nameModulo = preparation->nameModulo;
	root->nameTable = preparation->names;
	root->symbolModulo = preparation->symbolModulo;
	root->symbolTable = preparation->symbols;
	
	root->stack = &preparation->stack[0];
	root->stackBottom = &preparation->stack[0];
	root->stackTop = &preparation->stack[preparation->stackCount];
	
	root->firstHeap = &preparation->heap[0];
	root->freeHeap = &preparation->heap[preparation->heapCount - 1];
	root->aliasCount = (txID)preparation->aliasCount;
	
	screen->machine = fxCloneMachine(&preparation->creation, root, "mc", screen);
	if (!screen->machine) {
		return;
	}
	((txMachine*)(screen->machine))->host = screen;
	screen->idle = fxScreenIdle;
	screen->invoke = fxScreenInvoke;
	screen->quit = fxScreenQuit;
	screen->touch = fxScreenTouch;
#ifdef mxInstrument
	modInstrumentationInit();
	((txMachine*)(screen->machine))->onBreak = debugBreak;
	fxDescribeInstrumentation(screen->machine, screenInstrumentCount, screenInstrumentNames, screenInstrumentUnits);
	fxScreenSampleInstrumentation(screen);
#endif	
	xsBeginHost(screen->machine);
	{
		xsVars(2);
		xsCollectGarbage();
		xsVar(0) = xsNewHostObject(NULL); // no destructor
		xsSetHostData(xsVar(0), screen);
		xsVar(1) = xsNewHostFunction(screen_adaptInvalid, 0);
		xsDefine(xsVar(0), xsID("adaptInvalid"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_animateColors, 1);
		xsDefine(xsVar(0), xsID("animateColors"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_begin, 4);
		xsDefine(xsVar(0), xsID("begin"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_clear, 0);
		xsDefine(xsVar(0), xsID("clear"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_get_clut, 0);
		xsDefine(xsVar(0), xsID("clut"), xsVar(1), xsIsGetter);
		xsVar(1) = xsNewHostFunction(screen_set_clut, 1);
		xsDefine(xsVar(0), xsID("clut"), xsVar(1), xsIsSetter);
		xsVar(1) = xsNewHostFunction(screen_continue, 0);
		xsDefine(xsVar(0), xsID("continue"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_end, 0);
		xsDefine(xsVar(0), xsID("end"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_pixelsToBytes, 1);
		xsDefine(xsVar(0), xsID("pixelsToBytes"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_postMessage, 1);
		xsDefine(xsVar(0), xsID("postMessage"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_send, 1);
		xsDefine(xsVar(0), xsID("send"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_start, 1);
		xsDefine(xsVar(0), xsID("start"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_stop, 0);
		xsDefine(xsVar(0), xsID("stop"), xsVar(1), xsDefault);
		xsVar(1) = xsNewHostFunction(screen_get_pixelFormat, 0);
		xsDefine(xsVar(0), xsID("pixelFormat"), xsVar(1), xsIsGetter);
		xsVar(1) = xsNewHostFunction(screen_set_pixelFormat, 0);
		xsDefine(xsVar(0), xsID("pixelFormat"), xsVar(1), xsIsSetter);
		xsVar(1) = xsNewHostFunction(screen_get_width, 0);
		xsDefine(xsVar(0), xsID("width"), xsVar(1), xsIsGetter);
		xsVar(1) = xsNewHostFunction(screen_get_height, 0);
		xsDefine(xsVar(0), xsID("height"), xsVar(1), xsIsGetter);
#if kPocoFrameBuffer
		xsVar(1) = xsNewHostFunction(screen_get_frameBuffer, 0);
		xsDefine(xsVar(0), xsID("frameBuffer"), xsVar(1), xsIsGetter);
#endif
		xsSet(xsVar(0), xsID("pixelFormat"), xsInteger(kCommodettoBitmapFormat));
		xsSet(xsGlobal, xsID("screen"), xsVar(0));

		xsVar(0) = xsGet(xsGlobal, xsID("require"));
		xsVar(1) = xsCall1(xsVar(0), xsID("weak"), xsString("main"));
		if (xsTest(xsVar(1))) {
			if (xsIsInstanceOf(xsVar(1), xsFunctionPrototype)) {
				xsCallFunction0(xsVar(1), xsGlobal);
			}
			else if (xsFindResult(xsVar(1), xsID("onLaunch"))) {
				xsCallFunction0(xsResult, xsVar(1));
			}
		}
		
		xsCollectGarbage();
	}
	xsEndHost(screen->machine);
	(*screen->start)(screen, 5);
}

#ifdef mxInstrument
void fxScreenSampleInstrumentation(txScreen* screen)
{
	int what;
	txMachine *the = (txMachine*)screen->machine;
	for (what = kModInstrumentationPixelsDrawn; what <= kModInstrumentationPiuCommandListUsed; what++)
		screenInstrumentValues[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(what);
	fxSampleInstrumentation(screen->machine, screenInstrumentCount, screenInstrumentValues);
	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
	modInstrumentationSet(NetworkBytesRead, 0);
	modInstrumentationSet(NetworkBytesWritten, 0);
	the->garbageCollectionCount = 0;
	the->stackPeak = the->stack;
}
#endif

void fxScreenQuit(txScreen* screen)
{
	if (screen->machine) {
		(*screen->stop)(screen);
		xsBeginHost(screen->machine);
		{
			xsVars(2);
			xsVar(0) = xsGet(xsGlobal, xsID("screen"));
			xsVar(1) = xsGet(xsVar(0), xsID("context"));
			if (xsTest(xsVar(1))) {
				if (xsFindResult(xsVar(1), xsID("onQuit"))) {
					xsCallFunction0(xsResult, xsVar(1));
				}
			}
		}
		xsEndHost(screen->machine);
		xsDeleteMachine(screen->machine);
		screen->machine = NULL;
		screen->idle = NULL;
		screen->invoke = NULL;
		screen->quit = NULL;
		screen->touch = NULL;
		memset(screen->buffer, 0, screen->width * screen->height * screenBytesPerPixel);
	}
}

void fxScreenSetPalette(txScreen* screen, uint16_t* clut)
{
	uint8_t* component = screen->palette;
	int i = 0;
	while (i < 16) {
		uint16_t pixel = *clut;
		uint8_t r, g, b;
		r = pixel >> 11, g = (pixel >> 5) & 0x3F, b = pixel & 0x1F;
		*component++ = (r << 3) | (r >> 2);
		*component++ = (g << 2) | (g >> 4);
		*component++ = (b << 3) | (b >> 2);
		*component++ = 0xFF;
		clut++;
		i++;
	}
}

void fxScreenTouch(txScreen* screen, int kind, int index, int x, int y, double when)
{
	if (screen->machine) {
		xsBeginHost(screen->machine);
		{
			xsVars(2);
			xsVar(0) = xsGet(xsGlobal, xsID("screen"));
			xsVar(1) = xsGet(xsVar(0), xsID("context"));
			if (xsTest(xsVar(1))) {
				if (xsFindResult(xsVar(1), xsID(gxTouchEventNames[kind]))) {
					xsCallFunction4(xsResult, xsVar(1), xsInteger(index), xsInteger(x), xsInteger(y), xsNumber(when));
				}
			}
		}
		xsEndHost(screen->machine);
	}
}

void screen_adaptInvalid(xsMachine* the)
{
}

void screen_animateColors(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	uint8_t* components = screen->palette;
	uint8_t* p = screen->buffer;
	uint8_t* q = p + (screen->width * screen->height * screenBytesPerPixel);
	fxScreenSetPalette(screen, (uint16_t*)xsGetHostData(xsArg(0)));
	while (p < q) {
		uint8_t i = *(p + 3);
		uint8_t* component = components + (i << 2);
		*p++ = *component++;
		*p++ = *component++;
		*p++ = *component++;
		p++;
	}
	(*screen->bufferChanged)(screen);
}

void screen_begin(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	xsIntegerValue width = xsToInteger(xsArg(2));
	screen->rowAddress = screen->buffer + (y * screen->width * screenBytesPerPixel) + (x * screenBytesPerPixel);
	screen->rowCount = width;
	screen->rowDelta = (screen->width - width) * screenBytesPerPixel;
	screen->rowIndex = 0;
	//fprintf(stderr, "# BEGIN %ld %ld %ld %ld\n", x, y, width, height);

#if kPocoFrameBuffer
	xsResult = xsNewHostObject(NULL);
	xsSetHostData(xsResult, screen->frameBuffer);
	xsSet(xsResult, xsID("byteLength"), xsInteger(screen->frameBufferLength));
#endif
}

void screen_clear(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	memset(screen->buffer, 0, screen->width * screen->height * screenBytesPerPixel);
}

void screen_continue(xsMachine* the)
{
	//fprintf(stderr, "# CONTINUE\n");
}

void screen_end(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	//fprintf(stderr, "# END\n");

#if kPocoFrameBuffer
	screen->rowAddress = screen->buffer;
	screen->rowCount = screen->width;
	screen->rowDelta = 0;
	screen->rowIndex = 0;

	xsCall0(xsThis, xsID("send"));
#endif
	(*screen->bufferChanged)(screen);
}

void screen_pixelsToBytes(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	xsIntegerValue count = xsToInteger(xsArg(0));
	count *= gxPixelFormats[screen->pixelFormat].bitsPerPixel;
	count = (count + 7) >> 3;
	xsResult = xsInteger(count);
}

void screen_postMessage(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	if (screen->post) {
		if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
			char* buffer = (char*)xsToArrayBuffer(xsArg(0));
			int size = (int)xsGetArrayBufferLength(xsArg(0));
			(*screen->post)(screen, buffer, size);
		}
		else
			(*screen->post)(screen, xsToString(xsArg(0)), 0);
	}
}

void screen_send(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	unsigned char* rowAddress = screen->rowAddress;
	xsIntegerValue rowCount = screen->rowCount;
	xsIntegerValue rowDelta = screen->rowDelta;
	xsIntegerValue rowIndex = screen->rowIndex;
	xsIntegerValue c = xsToInteger(xsArgc);
	void *data;
	xsIntegerValue byteLength;
	xsIntegerValue offset;
	xsIntegerValue count;

	if (c) {
		data = (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) ? xsToArrayBuffer(xsArg(0)) : xsGetHostData(xsArg(0));
		byteLength = xsToInteger(xsGet(xsArg(0), xsID("byteLength")));
		offset = (c > 1) ? xsToInteger(xsArg(1)) : 0;
		count = (c > 2) ? xsToInteger(xsArg(2)) : byteLength - offset;
	}
	else {
#if kPocoFrameBuffer
		data = screen->frameBuffer;
		byteLength = screen->frameBufferLength;
		offset = 0;
		count = byteLength;
#else
		//@@ throw exception
#endif
	}
	//fprintf(stderr, "# SEND %p %ld %ld %ld\n", rowAddress, rowIndex, rowCount, rowDelta);
    switch (screen->pixelFormat) {
    case rgb565le: 
		{
			unsigned short* pixels = data;
			pixels += (offset >> 1);
			count >>= 1;
			while (count) {
				unsigned short pixel = *pixels++;
			#if mxLinux
				*rowAddress++ = (pixel & 0x001F) << 3;
				*rowAddress++ = (pixel & 0x07E0) >> 3;
				*rowAddress++ = (pixel & 0xF800) >> 8;
			#else
				*rowAddress++ = (pixel & 0xF800) >> 8;
				*rowAddress++ = (pixel & 0x07E0) >> 3;
				*rowAddress++ = (pixel & 0x001F) << 3;
			#endif
				*rowAddress++ = 0xFF;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
				}
				count--;
			}
		} 
		break;     
    case rgb565be:
    	{
			unsigned short* pixels = data;
			pixels += (offset >> 1);
			count >>= 1;
			while (count) {
				unsigned short pixel = *pixels++;
			#if mxLinux
				*rowAddress++ = (pixel & 0xF800) >> 8;
				*rowAddress++ = (pixel & 0x07E0) >> 3;
				*rowAddress++ = (pixel & 0x001F) << 3;
			#else
				*rowAddress++ = (pixel & 0x001F) << 3;
				*rowAddress++ = (pixel & 0x07E0) >> 3;
				*rowAddress++ = (pixel & 0xF800) >> 8;
			#endif
				*rowAddress++ = 0xFF;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
				}
				count--;
			}
		} 
    	break;     
    case gray8:
    	{
  			unsigned char* pixels = data;
			pixels += offset;
			while (count) {
				unsigned char pixel = *pixels++;
				*rowAddress++ = pixel;
				*rowAddress++ = pixel;
				*rowAddress++ = pixel;
				*rowAddress++ = 0xFF;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
				}
				count--;
			}
		}
    	break;
    case rgb332:
    	{
  			unsigned char* pixels = data;
			pixels += offset;
			while (count) {
				unsigned char pixel = *pixels++;
				unsigned char r, g, b;
				r = pixel >> 5;
				g = (pixel >> 2) & 7;
				b = pixel & 3;
			#if mxLinux
				*rowAddress++ = (b << 6) | (b << 4) | (b << 2) | b;
				*rowAddress++ = (g << 5) | (g << 2) | (g >> 1);
				*rowAddress++ = (r << 5) | (r << 2) | (r >> 1);
			#else
				*rowAddress++ = (r << 5) | (r << 2) | (r >> 1);
				*rowAddress++ = (g << 5) | (g << 2) | (g >> 1);
				*rowAddress++ = (b << 6) | (b << 4) | (b << 2) | b;
			#endif
				*rowAddress++ = 0xFF;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
				}
				count--;
			}
		}
    	break;
    case gray4:
    	{
  			unsigned char* pixels = data;
			pixels += offset;
			while (count--) {
				unsigned char twoPixels = *pixels++;
				unsigned char pixel;

				pixel = (twoPixels & 0xF0) | (twoPixels >> 4);
				*rowAddress++ = pixel;
				*rowAddress++ = pixel;
				*rowAddress++ = pixel;
				*rowAddress++ = 0xFF;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
					continue;		// scan line begins on byte boundary
				}

				pixel = (twoPixels & 0x0F) | (twoPixels << 4);
				*rowAddress++ = pixel;
				*rowAddress++ = pixel;
				*rowAddress++ = pixel;
				*rowAddress++ = 0xFF;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
				}
			}
		}
    	break;
    case clut4:
    	{
    		uint8_t* components = screen->palette;
  			unsigned char* pixels = data;
			pixels += offset;
			while (count--) {
				uint8_t twoPixels = *pixels++;
#if kPocoCLUT16_01
				uint8_t index = twoPixels >> 4;
#else
				uint8_t index = twoPixels & 0x0F;
#endif
				uint8_t* component = components + (index << 2);
			#if mxLinux
				*rowAddress++ = component[2];
				*rowAddress++ = component[1];
				*rowAddress++ = component[0];
			#else
				*rowAddress++ = *component++;
				*rowAddress++ = *component++;
				*rowAddress++ = *component++;
			#endif
				*rowAddress++ = index;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
					continue;		// scan line begins on byte boundary
				}
#if kPocoCLUT16_01
				index = twoPixels & 0x0F;
#else
				index = twoPixels >> 4;
#endif
				component = components + (index << 2);
			#if mxLinux
				*rowAddress++ = component[2];
				*rowAddress++ = component[1];
				*rowAddress++ = component[0];
			#else
				*rowAddress++ = *component++;
				*rowAddress++ = *component++;
				*rowAddress++ = *component++;
			#endif
				*rowAddress++ = index;
				rowIndex++;
				if (rowIndex == rowCount) {
					rowIndex = 0;
					rowAddress += rowDelta;
				}
			}
		}
		break;
    }
	screen->rowAddress = rowAddress;
	screen->rowIndex = rowIndex;
}

void screen_start(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	screen->flags |= mxScreenIdling;
}

void screen_stop(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	screen->flags &= ~mxScreenIdling;
}

void screen_get_clut(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	if (!screen->clut)
		return;
	xsResult = xsNewHostObject(NULL);
	xsSetHostData(xsResult, screen->clut);
}

void screen_get_pixelFormat(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	xsResult = xsInteger(gxPixelFormats[screen->pixelFormat].commodettoFormat);
}

void screen_get_width(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	xsResult = xsInteger(screen->width);	
}

void screen_get_height(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	xsResult = xsInteger(screen->height);	
}

void screen_set_clut(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	screen->clut = xsGetHostData(xsArg(0));		// cannot be array buffer
	//@@ check length
	screen_clear(the);
	fxScreenSetPalette(screen, (uint16_t *)(32 + 4096 + (char *)screen->clut));
}

void screen_set_pixelFormat(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsThis);
	xsIntegerValue commodettoFormat = xsToInteger(xsArg(0));
	xsIntegerValue i;
	for (i = 0; i < pixelFormatCount; i++) {
		if (gxPixelFormats[i].commodettoFormat == commodettoFormat)
			break;
	}
	if (i == pixelFormatCount)
		xsErrorPrintf("unsupported pixel format");
	screen->pixelFormat = i;
	(*screen->formatChanged)(screen);

#if kPocoFrameBuffer
	if (screen->frameBuffer)
		free(screen->frameBuffer);

	screen->frameBufferLength = screen->height * ((screen->width * gxPixelFormats[screen->pixelFormat].bitsPerPixel) >> 3);
	screen->frameBuffer = malloc(screen->frameBufferLength);
#endif
}

#if kPocoFrameBuffer
void screen_get_frameBuffer(xsMachine* the)
{
	xsResult = xsTrue;
}
#endif

xsBooleanValue fxFindResult(xsMachine* the, xsSlot* slot, xsIndex id)
{
	xsBooleanValue result;
	xsOverflow(-1);
	fxPush(*slot);
	if (fxHasID(the, id)) {
		fxPush(*slot);
		fxGetID(the, id);
		xsResult = *the->stack;
		the->stack++;
		result = 1;
	}
	else
		result = 0;
	return result;
}

xsBooleanValue fxArchiveRead(void* src, size_t offset, void* buffer, size_t size)
{
	c_memcpy(buffer, ((txU1*)src) + offset, size);
	return 1;
}

xsBooleanValue fxArchiveWrite(void* dst, size_t offset, void* buffer, size_t size)
{
	c_memcpy(((txU1*)dst) + offset, buffer, size);
	return 1;
}

void Worker_destructor(void* data)
{
}

void Worker_constructor(xsMachine* the)
{
	txScreen* screen = xsGetHostData(xsGet(xsGlobal, xsID_screen));
	txWorker* worker = NULL;
	if (!screen->createWorker) goto err;
	worker = c_malloc(sizeof(txWorker));
	if (!worker) goto err;
	worker->id = (*screen->createWorker)(screen, xsToString(xsArg(0)));
	if (!worker->id) goto err;
	worker->slot =xsThis;
	xsRemember(worker->slot);
	worker->screen = screen;
	worker->nextWorker = screen->firstWorker;
	screen->firstWorker = worker;
	xsSetHostData(xsThis, worker);
	return;
err:
	if (worker)
		c_free(worker);
	xsUnknownError("unable to instantiate worker");

}

void Worker_postMessage(xsMachine* the)
{
	txWorker* worker = xsGetHostData(xsThis);
	if (worker) {
		txScreen* screen = worker->screen;
		if (screen->post)
			(*screen->post)(screen, xsMarshallAlien(xsArg(0)), 0 - worker->id);
	}
}

void Worker_terminate(xsMachine* the)
{
	txWorker* worker = xsGetHostData(xsThis);
	if (worker) {
		txScreen* screen = worker->screen;
		txWorker **address = (txWorker**)&screen->firstWorker, *current;
		while ((current = *address)) {
			if (current == worker) {
				*address = worker->nextWorker;
				break;
			}
			address = &current->nextWorker;
		}	
		xsForget(worker->slot);
		if (screen->deleteWorker) 
			(*screen->deleteWorker)(screen, worker->id);
		c_free(worker);
		xsSetHostData(xsThis, NULL);
	}
}


