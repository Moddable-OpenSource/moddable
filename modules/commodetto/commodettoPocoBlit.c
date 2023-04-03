/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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


/*

	Note: kCommodettoBitmapCLUT16 implementation is inconsistent when kPocoCLUT16_01 is 0. Source pixels are stored in
		the opposite order within a byte from destination pixels. Consequently, rendering to an offscreen will generate incorrect results.
*/

#include "commodettoPocoBlit.h"

#include "xsPlatform.h"

enum {
	kPocoCommandRectangleFill = 0,		// must start at 0 to match gDrawRenderCommand
	kPocoCommandRectangleBlend,
	kPocoCommandPixelDraw,
	kPocoCommandBitmapDraw,
	kPocoCommandMonochromeBitmapDraw,
	kPocoCommandMonochromeForegroundBitmapDraw,
	kPocoCommandGray16BitmapDraw,
	kPocoCommandGray16RLEBitmapDraw,
	kPocoCommandGray16RLEBlendBitmapDraw,
	kPocoCommandBitmapDrawMasked,
	kPocoCommandBitmapPattern,
	kPocoCommandFrame,
	kPocoCommandExternal,
	kPocoCommandDrawMax = kPocoCommandExternal + 1
};

typedef uint8_t PocoCommandID;

#if kPocoPixelSize >= 8
	#define PocoCommandFields		\
		PocoCommandID	command;	\
		uint8_t			length;		\
		PocoCoordinate	x;			\
		PocoCoordinate	y;			\
		PocoDimension	w;			\
		PocoDimension	h
#else
	#define PocoCommandFields		\
		PocoCommandID	command;	\
		uint8_t			length;		\
		PocoCoordinate	x;			\
		PocoCoordinate	y;			\
		PocoDimension	w;			\
		PocoDimension	h;			\
		uint8_t			xphase
#endif

typedef void (*PocoRenderCommandProc)(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);

static void doFillRectangle(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doBlendRectangle(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h);
static void doDrawPixel(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doDrawBitmap(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
//static void doDrawPackedBitmapUnclipped(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
//static void doDrawPackedBitmapClipped(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doDrawMonochromeBitmapPart(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doDrawMonochromeForegroundBitmapPart(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doDrawGray16BitmapPart(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h);
static void doDrawGray16RLEBitmapPart(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h);
static void doDrawGray16RLEBlendBitmapPart(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h);
static void doDrawMaskedBitmap(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h);
static void doDrawPattern(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doDrawFrame(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);
static void doDrawExternal(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h);

static uint8_t doSkipColorCells(Poco poco, PocoCommand pc, int cells);

static const PocoRenderCommandProc gDrawRenderCommand[kPocoCommandDrawMax] ICACHE_RODATA_ATTR = {
	doFillRectangle,
	doBlendRectangle,
	doDrawPixel,
	doDrawBitmap,
	doDrawMonochromeBitmapPart,
	doDrawMonochromeForegroundBitmapPart,
	doDrawGray16BitmapPart,
	doDrawGray16RLEBitmapPart,
	doDrawGray16RLEBlendBitmapPart,
	doDrawMaskedBitmap,
	doDrawPattern,
	doDrawFrame,
	doDrawExternal
};

#if !kPocoFrameBuffer
	#define PocoCommandBuilt(poco, pc) (poco)->next = (PocoCommand)((pc)->length + (char *)(pc))
#elif 4 != kPocoPixelSize
	#define PocoCommandBuilt(poco, pc) \
		do { \
			if (poco->frameBuffer) \
				(gDrawRenderCommand[pc->command])(poco, pc, (PocoPixel *)(((pc)->y * poco->rowBytes) + (char *)(poco->frameBuffer + (pc)->x)), (pc)->h); \
			else \
				(poco)->next = (PocoCommand)((pc)->length + (char *)(pc)); \
		} while (0)
#elif 4 == kPocoPixelSize
	#define PocoCommandBuilt(poco, pc) \
		do { \
			if (poco->frameBuffer) { \
				(pc)->xphase = (pc)->x & 1; \
				(gDrawRenderCommand[pc->command])(poco, pc, (PocoPixel *)(((pc)->y * poco->rowBytes) + (char *)(poco->frameBuffer + ((pc)->x >> 1))), (pc)->h); \
			} \
			else \
				(poco)->next = (PocoCommand)((pc)->length + (char *)(pc)); \
		} while (0)
#endif

#define PocoCommandLength(type) ((3 + (sizeof(type))) & ~3)
#define PocoCommandSetLength(pc, value) (pc)->length = ((3 + (value)) & ~3)

struct PocoCommandRecord {
	PocoCommandFields;
};

#define PocoReturnIfNoSpace(DISPLAY_LIST_POSITION, bytes) \
	if (((char *)poco->displayListEnd - (char *)(DISPLAY_LIST_POSITION)) < (int)(bytes)) {	\
		poco->flags |= kPocoFlagErrorDisplayListOverflow;										\
		pocoInstrumentationMax(PocoDisplayListUsed, (char *)poco->next - (char *)poco->displayList);							\
		return;																		\
	}

#if defined(__GNUC__)
	#define SwapLong(a) __builtin_bswap32(a)
#else
	#define SwapLong(a) (a >> 24) | ((a >> 8) & 0x0000ff00) | ((a & 0x0000ff00) << 8) | (a << 24)
#endif

/*
	memcpy variants
*/

#if 4 == kPocoPixelSize
	#if kPocoPixelFormat == kCommodettoBitmapGray16
		typedef void (*PocoCopy4)(uint8_t *to, const uint8_t *from, uint16_t pixels);

		static void copy4AlignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels);
		static void copy4MisalignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels);
		static void copy4AlignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels);
		static void copy4MisalignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels);
	#else
		typedef void (*PocoCopy4)(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap);

		static void copy4AlignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap);
		static void copy4MisalignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap);
		static void copy4AlignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap);
		static void copy4MisalignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap);
	#endif

	static const PocoCopy4 gCopy4[4] ICACHE_RODATA_ATTR = {
		copy4AlignedSourceAlignedDest,
		copy4AlignedSourceMisalignedDest,
		copy4MisalignedSourceAlignedDest,
		copy4MisalignedSourceMisalignedDest
	};

	#define getMemCpy4(sx, dx) (gCopy4[(((sx) & 1) << 1) | ((dx) & 1)])

	#if kPocoPixelFormat == kCommodettoBitmapCLUT16
		static void buildColorMap(uint32_t *srcCLUT, uint8_t *inverseTable, uint8_t *remap);
	#endif
#endif

/*
	table to convert 4-bit alpha level to 5 bit blend through 4-bit blend option

		for (let blend = 0; blend < 16; blend++) {
			let values = [];
			let max = 31 * (blend / 15);
			for (i = 0; i < 16; i++)
				values[15 - i] = Math.round((i / 15) * max);
			console.log(values.join(", "));
		}
*/

#if kPocoPixelSize >= 8
	static const uint8_t gBlenders[256] ICACHE_XS6RO_ATTR = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
		4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0,
		6, 6, 5, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 0, 0,
		8, 8, 7, 7, 6, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0,
		10, 10, 9, 8, 8, 7, 6, 6, 5, 4, 3, 3, 2, 1, 1, 0,
		12, 12, 11, 10, 9, 8, 7, 7, 6, 5, 4, 3, 2, 2, 1, 0,
		14, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
		17, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 4, 3, 2, 1, 0,
		19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 4, 2, 1, 0,
		21, 19, 18, 17, 15, 14, 12, 11, 10, 8, 7, 6, 4, 3, 1, 0,
		23, 21, 20, 18, 17, 15, 14, 12, 11, 9, 8, 6, 5, 3, 2, 0,
		25, 23, 21, 20, 18, 17, 15, 13, 12, 10, 8, 7, 5, 3, 2, 0,
		27, 25, 23, 21, 20, 18, 16, 14, 13, 11, 9, 7, 5, 4, 2, 0,
		29, 27, 25, 23, 21, 19, 17, 15, 14, 12, 10, 8, 6, 4, 2, 0,
		31, 29, 27, 25, 23, 21, 19, 17, 14, 12, 10, 8, 6, 4, 2, 0
	};
#else
	static const uint8_t gBlenders[256] ICACHE_XS6RO_ATTR = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
		3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0,
		4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0,
		5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0,
		6, 6, 5, 5, 4, 4, 4, 3, 3, 2, 2, 2, 1, 1, 0, 0,
		7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0,
		8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0,
		9, 8, 8, 7, 7, 6, 5, 5, 4, 4, 3, 2, 2, 1, 1, 0,
		10, 9, 9, 8, 7, 7, 6, 5, 5, 4, 3, 3, 2, 1, 1, 0,
		11, 10, 10, 9, 8, 7, 7, 6, 5, 4, 4, 3, 2, 1, 1, 0,
		12, 11, 10, 10, 9, 8, 7, 6, 6, 5, 4, 3, 2, 2, 1, 0,
		13, 12, 11, 10, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 0,
		14, 13, 12, 11, 10, 9, 8, 7, 7, 6, 5, 4, 3, 2, 1, 0,
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
	};
#endif

#if !defined(kPocoCLUT16_01) || kPocoCLUT16_01
	// first pixel is in high nybble (4-bit format in BMP)
	#undef kPocoCLUT16_01
	#define kPocoCLUT16_01 1
	#define kPocoPixels4FirstMask (0xF0)
	#define kPocoPixels4FirstShift (4)
	#define kPocoPixels4SecondMask (0x0F)
	#define kPocoPixels4SecondShift (0)
	#define PocoPixels4IsFirstPixel(xphase) (!(xphase))
	#define PocoPixels4IsSecondPixel(xphase) (xphase)
#else
	// first pixel is in low nybble (4-bit format used by D/AVE 2D engine)
	#undef kPocoCLUT16_01
	#define kPocoCLUT16_01 0
	#define kPocoPixels4FirstMask (0x0F)
	#define kPocoPixels4FirstShift (0)
	#define kPocoPixels4SecondMask (0xF0)
	#define kPocoPixels4SecondShift (4)
	#define PocoPixels4IsFirstPixel(xphase) (xphase)
	#define PocoPixels4IsSecondPixel(xphase) (!(xphase))
#endif
#define PocoPixels4Pack(first, second) (((first) << kPocoPixels4FirstShift) | ((second) << kPocoPixels4SecondShift))

/*
	here begin the functions to build the drawing list
*/

typedef struct ColorDrawRecord {
	PocoCommandFields;

	PocoPixel	color;
} ColorDrawRecord, *ColorDraw;

typedef struct BlendDrawRecord {
	PocoCommandFields;

	PocoPixel	color;
	uint8_t		blend;
} BlendDrawRecord, *BlendDraw;

typedef struct RenderBitsRecord {
	PocoCommandFields;

#if 4 != kPocoPixelSize
	uint16_t	rowPixels;
	void		*pixels;
#else
	uint16_t	rowBytes;
	void		*pixels;
	PocoCopy4	doCopy;
	#if kPocoPixelFormat == kCommodettoBitmapCLUT16
		uint8_t	remap[16];		//@@ HUGE... need a cache of these
	#endif
#endif
} RenderBitsRecord, *RenderBits;

typedef struct RenderPackedBitsUnclippedRecord {
	PocoCommandFields;

	const void	*pixels;
} RenderPackedBitsUnclippedRecord, *RenderPackedBitsUnclipped;

typedef struct RenderPackedBitsClippedRecord {
	PocoCommandFields;

	const unsigned char	*pixels;
	PocoCoordinate		left;
	PocoCoordinate		right;
} RenderPackedBitsClippedRecord, *RenderPackedBitsClipped;

typedef struct RenderMonochromeBitsRecord {
	PocoCommandFields;

	uint16_t			rowBump;
	const unsigned char	*pixels;
	PocoPixel			fore;
	uint8_t				mask;
	uint8_t				mode;		// record truncated here in foreground only mode
	PocoPixel			back;
} RenderMonochromeBitsRecord, *RenderMonochromeBits;

typedef struct RenderGray16BitsRecord {
	PocoCommandFields;

	uint8_t				mask;
	uint8_t				blendersOffset;
	uint16_t			rowBump;
	PocoColor			color;
	const unsigned char	*pixels;
} RenderGray16BitsRecord, *RenderGray16Bits;

typedef struct RenderGray16RLEBitsRecord {
	PocoCommandFields;

	uint8_t				nybbleCount;			// 0 to 7
	uint8_t				blendersOffset;
	PocoColor			color;
	const uint8_t		*pixels;

	// placement and order of thisSkip and skip fields is signficant - they are ommited for unclipped blits
	PocoCoordinate		thisSkip;
	PocoCoordinate		skip;					// left clip + right clip
} RenderGray16RLEBitsRecord, *RenderGray16RLEBits;

typedef struct RenderMaskedBitsRecord {
	PocoCommandFields;

	uint16_t	rowBump;
	const void	*pixels;

	uint16_t	maskBump;
	const void	*maskBits;
	uint8_t		mask;
	uint8_t		blendersOffset;

#if 4 == kPocoPixelSize
	uint8_t		xmask_src;
#if kPocoPixelFormat == kCommodettoBitmapCLUT16
	uint8_t	remap[16];		//@@ HUGE... need a cache of these
#endif
#endif
} RenderMaskedBitsRecord, *RenderMaskedBits;

typedef struct PatternBitsRecord {
	PocoCommandFields;

	int16_t		rowBump;

	const void	*pixels;

	const PocoPixel *patternStart;

	uint16_t	xOffset;
	uint16_t	dy;
	uint16_t	patternW;
	uint16_t	patternH;

#if kPocoPixelFormat == kCommodettoBitmapCLUT16
	uint8_t	remap[16];		//@@ HUGE... need a cache of these
#endif
} PatternBitsRecord, *PatternBits;

typedef struct FrameRecord {
	PocoCommandFields;

	const uint8_t	*data;

#if kPocoPixelFormat != kCommodettoBitmapCLUT16
	PocoPixel		prev0;
	PocoPixel		prev1;
#else
	uint16_t		prev0;
	uint16_t		prev1;
#endif
	uint16_t		clipLeft;
	uint8_t			yOffset;
	uint8_t			blockWidth;
	uint8_t			unclippedBlockWidth;
} FrameRecord, *Frame;

typedef struct ExternalRecord {
	PocoCommandFields;

	PocoRenderExternal	doDrawExternal;
	uint8_t				data[1];
} ExternalRecord, *External;

void PocoRectangleFill(Poco poco, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PocoCommand pc = poco->next;
	PocoCoordinate xMax, yMax;

	PocoReturnIfNoSpace(pc, sizeof(BlendDrawRecord));		// BlendDraw is worst case

	rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);

	xMax = x + w;
	yMax = y + h;

	if (x < poco->x)
		x = poco->x;

	if (xMax > poco->xMax)
		xMax = poco->xMax;

	if (x >= xMax)
		return;

	w = xMax - x;

	if (y < poco->y)
		y = poco->y;

	if (yMax > poco->yMax)
		yMax = poco->yMax;

	if (y >= yMax)
		return;

	h = yMax - y;

	if (kPocoOpaque == blend) {
		pc->command = kPocoCommandRectangleFill;
		PocoCommandSetLength(pc, sizeof(ColorDrawRecord));
#if kCommodettoBitmapFormat != kCommodettoBitmapCLUT16
		((ColorDraw)pc)->color = color;
#else
		((ColorDraw)pc)->color = c_read8(poco->clut + 32 + color);
#endif
	}
	else {
		BlendDraw bd = (BlendDraw)pc;

		if (0 == blend)
			return;

		bd->command = kPocoCommandRectangleBlend;
		PocoCommandSetLength(bd, sizeof(BlendDrawRecord));
#if kCommodettoBitmapFormat != kCommodettoBitmapCLUT16
		bd->color = color;
#else
		bd->color = c_read8(poco->clut + 32 + color);
#endif
		bd->blend = blend >> 3;		// 5 bit blend level
	}
	pc->x = x, pc->y = y, pc->w = w, pc->h = h;

	PocoCommandBuilt(poco, pc);
}

void PocoPixelDraw(Poco poco, PocoColor color, PocoCoordinate x, PocoCoordinate y)
{
	PocoCommand pc = poco->next;

	PocoReturnIfNoSpace(pc, sizeof(ColorDrawRecord));

	rotateCoordinates(poco->width, poco->height, x, y, 1, 1);

	if ((x < poco->x) || (y < poco->y) || (x >= poco->xMax) || (y >= poco->yMax))
		return;

	pc->command = kPocoCommandPixelDraw;
	PocoCommandSetLength(pc, sizeof(ColorDrawRecord));
	pc->x = x, pc->y = y, pc->w = 1, pc->h = 1;
#if kCommodettoBitmapFormat != kCommodettoBitmapCLUT16
	((ColorDraw)pc)->color = color;
#else
	((ColorDraw)pc)->color = c_read8(poco->clut + 32 + color);
#endif

	PocoCommandBuilt(poco, pc);
}

// this function does not work in all cases. it is designed for these two situations
//	1. full scan lines (start to end)
//	2. parsing to the end of a scan line
const unsigned char *skipPackedPixels(const unsigned char *pixels, uint32_t skipPixels)
{  
	while (skipPixels) {
		signed char opcode = *pixels++;
		unsigned char count = (opcode & 0x3F) + 1;
		if (opcode < 0) {
			if (!(opcode & 0x40))	// repeat (not skip)
				pixels += sizeof(PocoPixel);
		}
		else						// literal
			pixels += count * sizeof(PocoPixel);
		skipPixels -= count;
	}
	return pixels;
}

void PocoBitmapDraw(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	const PocoPixel *pixels;
	int16_t d;
	PocoCommand pc = poco->next;

	PocoReturnIfNoSpace(pc, sizeof(RenderMonochromeBitsRecord));

	rotateCoordinates(poco->width, poco->height, x, y, sw, sh);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
	rotateCoordinatesAndDimensions(bits->width, bits->height, sx, sy, sw, sh);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
	rotateCoordinatesAndDimensions(bits->height, bits->width, sx, sy, sw, sh);
#endif

	if ((x >= poco->xMax) || (y >= poco->yMax))
		return;

	if (x < poco->x) {
		d = poco->x - x;
		if (sw <= d)
			return;
		sx += d;
		sw -= d;
		x = poco->x;
	}

	if (y < poco->y) {
		d = poco->y - y;
		if (sh <= d)
			return;
		sy += d;
		sh -= d;
		y = poco->y;
	}

	if ((x + sw) > poco->xMax)
		sw = poco->xMax - x;

	if ((y + sh) > poco->yMax)
		sh = poco->yMax - y;

	if ((sx >= bits->width) || (sy >= bits->height) || ((sx + sw) > bits->width) || ((sy + sh) > bits->height) || !sw || !sh)
		return;

	pixels = bits->pixels;

	pc->x = x, pc->y = y, pc->w = sw, pc->h = sh;

	if (kCommodettoBitmapFormat == bits->format) {
		pc->command = kPocoCommandBitmapDraw;
		PocoCommandSetLength(pc, sizeof(RenderBitsRecord));
#if 4 != kPocoPixelSize
		((RenderBits)pc)->pixels = (void *)(pixels + (sy * bits->width) + sx);
		((RenderBits)pc)->rowPixels = bits->width;
#else
		((RenderBits)pc)->pixels = (void *)(pixels + ((sy * (bits->width + 1)) >> 1) + (sx >> 1));
		((RenderBits)pc)->rowBytes = (bits->width + 1) >> 1;
		((RenderBits)pc)->doCopy = getMemCpy4(sx, x - poco->x);
#if kPocoPixelFormat == kCommodettoBitmapCLUT16
		buildColorMap((uint32_t *)((uint8_t *)bits->pixels) - 16, poco->clut + 32, ((RenderBits)pc)->remap);
#endif
#endif
	}
	else
	if (kCommodettoBitmapMonochrome == bits->format) {
		uint8_t mask;
		RenderMonochromeBits srcBits = (RenderMonochromeBits)pc;

		pixels = (PocoPixel *)(((char *)pixels) + ((bits->width + 7) >> 3) * sy);  // clipped off top. skip sy scan lines.
		pixels = (PocoPixel *)(((char *)pixels) + (sx >> 3));
		sx &= 0x07;
		mask = 1 << (7 - sx);

		pc->command = kPocoCommandMonochromeBitmapDraw;
		PocoCommandSetLength(pc, sizeof(RenderMonochromeBitsRecord));
		srcBits->pixels = (const unsigned char *)pixels;
		srcBits->rowBump = (bits->width + 7) >> 3;
		srcBits->mask = mask;
		srcBits->mode = kPocoMonochromeForeAndBackground;	// mode, fore, amd back are patched in PocoMonochromeBitmapDraw
		srcBits->fore = 0x00;
		srcBits->back = ~0;
// 		return;		// special case for PocoMonochromeBitmapDraw
	}
	else
		return;

	PocoCommandBuilt(poco, pc);
}

void PocoMonochromeBitmapDraw(Poco poco, PocoBitmap bits, PocoMonochromeMode mode, PocoColor fgColor, PocoColor bgColor, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoCommand pc = poco->next;
	RenderMonochromeBits rmb = (RenderMonochromeBits)pc;

	if (kCommodettoBitmapMonochrome != bits->format)
		return;

	PocoBitmapDraw(poco, bits, x, y, sx, sy, sw, sh);

	if (poco->next == pc)
		return;		// didn't queue anything to draw

	rmb->mode = mode;
#if kPocoPixelFormat != kCommodettoBitmapCLUT16
	rmb->fore = fgColor;
	rmb->back = bgColor;
#else
	rmb->fore = ((ColorDraw)pc)->color = c_read8(poco->clut + 32 + fgColor);
	rmb->back = ((ColorDraw)pc)->color = c_read8(poco->clut + 32 + bgColor);
#endif
	if (kPocoMonochromeForeground == mode) {
		pc->command = kPocoCommandMonochromeForegroundBitmapDraw;
		PocoCommandSetLength(pc, offsetof(RenderMonochromeBitsRecord, mode));	// truncate mode and background color, unused by foreground-only blitter
	}

	PocoCommandBuilt(poco, pc);
}

void PocoGrayBitmapDraw(Poco poco, PocoBitmap bits, PocoColor color, uint8_t blend, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoCommand pc = poco->next;
	int16_t d;

	if (kCommodettoBitmapGray16 != (bits->format & ~kCommodettoBitmapPacked)) {
		if (kCommodettoBitmapMonochrome == bits->format)
			PocoMonochromeBitmapDraw(poco, bits, kPocoMonochromeForeground, color, color, x, y, sx, sy, sw, sh);
		return;
	}

	PocoReturnIfNoSpace(pc, sizeof(RenderGray16BitsRecord));

	rotateCoordinates(poco->width, poco->height, x, y, sw, sh);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
	rotateCoordinatesAndDimensions(bits->width, bits->height, sx, sy, sw, sh);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
	rotateCoordinatesAndDimensions(bits->height, bits->width, sx, sy, sw, sh);
#endif
	if ((x >= poco->xMax) || (y >= poco->yMax) || (0 == blend))
		return;

	if (x < poco->x) {
		d = poco->x - x;
		if (sw <= d)
			return;
		sx += d;
		sw -= d;
		x = poco->x;
	}

	if (y < poco->y) {
		d = poco->y - y;
		if (sh <= d)
			return;
		sy += d;
		sh -= d;
		y = poco->y;
	}

	if ((x + sw) > poco->xMax)
		sw = poco->xMax - x;

	if ((y + sh) > poco->yMax)
		sh = poco->yMax - y;

	if ((sx >= bits->width) || (sy >= bits->height) || ((sx + sw) > bits->width) || ((sy + sh) > bits->height) || !sw || !sh)
		return;

	pc->x = x, pc->y = y, pc->w = sw, pc->h = sh;

	if (!(bits->format & kCommodettoBitmapPacked)) {
		RenderGray16Bits srcBits;
		const PocoPixel *pixels = bits->pixels;

		pixels = (PocoPixel *)(((char *)pixels) + ((bits->width >> 1) * sy));  // clipped off top. skip sy scan lines.
		pixels = (PocoPixel *)(((char *)pixels) + (sx >> 1));

		pc->command = kPocoCommandGray16BitmapDraw;
		PocoCommandSetLength(pc, sizeof(RenderGray16BitsRecord));
		srcBits = (RenderGray16Bits)pc;
		srcBits->pixels = (const unsigned char *)pixels;
	//		srcBits->rowBump = ((bits->width + 1) >> 1) - ((((sx & 1) + sw) + 1) >> 1);
		srcBits->rowBump = bits->width >> 1;
		srcBits->mask = (sx & 1) ? 0 : 4;
		srcBits->color = color;
		srcBits->blendersOffset = blend & 0xF0;		// (blend >> 4) << 4
	}
	else {
		RenderGray16RLEBits srcBits = (RenderGray16RLEBits)pc;
		uint32_t nybbles;
		const uint32_t *pixels = (uint32_t *)bits->pixels;
		uint8_t nybbleCount = (4 - (3 & (uintptr_t)pixels)) << 1;
		pixels = (uint32_t *)(~3 & ((uintptr_t)pixels));
		nybbles = *pixels++;
		nybbles >>= (32 - (nybbleCount << 2));

		if (kPocoOpaque == blend)
			pc->command = kPocoCommandGray16RLEBitmapDraw;
		else {
			pc->command = kPocoCommandGray16RLEBlendBitmapDraw;
			((RenderGray16RLEBits)pc)->blendersOffset = blend & 0xF0;		// (blend >> 4) << 4
		}

		if (bits->width - sw) {
			PocoCommandSetLength(pc, sizeof(RenderGray16RLEBitsRecord));
			srcBits->thisSkip = sx;
			srcBits->skip = bits->width - sw;
		}
		else
			PocoCommandSetLength(pc, offsetof(RenderGray16RLEBitsRecord, thisSkip));

		while (sy--) {
			PocoDimension remaining = bits->width;

			do {
				uint8_t nybble;

				if (0 == nybbleCount) {
					nybbles = *pixels++;
					nybbleCount = 8;
				}

				nybble = nybbles & 0x0F;
				nybbles >>= 4;
				nybbleCount -= 1;

				if (8 & nybble) {
					if (0x08 == (nybble & 0x0C))
						remaining -= (nybble & 3) + 2;	// solid
					else {		// quote
						uint8_t count = (nybble & 3) + 1;
						remaining -= count;

						if (count <= nybbleCount)
							nybbleCount -= count;
						else {
							nybbles = *pixels++;
							count -= nybbleCount;
							nybbleCount = 8 - count;
						}
						nybbles >>= (count << 2);
					}
				}
				else
					remaining -= nybble + 2;		// skip
			} while (remaining);
		}

		srcBits->pixels = (const uint8_t *)pixels;
		srcBits->nybbleCount = nybbleCount;
		srcBits->color = color;
	}

	PocoCommandBuilt(poco, pc);
}

void PocoBitmapDrawMasked(Poco poco, uint8_t blend, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh,
			PocoBitmap mask, PocoDimension mask_sx, PocoDimension mask_sy)
{
	const uint8_t *maskBits;
	PocoCommand pc = poco->next;
	int16_t d;

	PocoReturnIfNoSpace(pc, sizeof(RenderMaskedBitsRecord));

	rotateCoordinates(poco->width, poco->height, x, y, sw, sh);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
	rotateCoordinates(mask->width, mask->height, mask_sx, mask_sy, sw, sh);
	rotateCoordinatesAndDimensions(bits->width, bits->height, sx, sy, sw, sh);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
	rotateCoordinates(mask->height, mask->width, mask_sx, mask_sy, sw, sh);
	rotateCoordinatesAndDimensions(bits->height, bits->width, sx, sy, sw, sh);
#endif

	if (x < poco->x) {
		d = poco->x - x;
		if (sw <= d)
			return;
		sx += d;
		sw -= d;
		mask_sx += d;
		x = poco->x;
	}

	if (y < poco->y) {
		d = poco->y - y;
		if (sh <= d)
			return;
		sy += d;
		sh -= d;
		mask_sy += d;
		y = poco->y;
	}

	if ((x + sw) > poco->xMax)
		sw = poco->xMax - x;

	if ((y + sh) > poco->yMax)
		sh = poco->yMax - y;

	if ((mask_sx >= mask->width) || ((mask_sx + sw) > mask->width) || (mask_sy >= mask->height) || ((mask_sy + sh) > mask->height) || !sw || !sh || !blend)
		return;

	pc->x = x, pc->y = y, pc->w = sw, pc->h = sh;

	pc->command = kPocoCommandBitmapDrawMasked;
	PocoCommandSetLength(pc, sizeof(RenderMaskedBitsRecord));
#if 4 != kPocoPixelSize
	((RenderMaskedBits)pc)->pixels = (void *)(bits->pixels + (sy * bits->width) + sx);
	((RenderMaskedBits)pc)->rowBump = bits->width - sw;
#else
	((RenderMaskedBits)pc)->pixels = (void *)(bits->pixels + (((sy * bits->width) + sx) >> 1));
	((RenderMaskedBits)pc)->rowBump = (bits->width - sw + ((sw ^ sx) & 1)) >> 1;
	((RenderMaskedBits)pc)->xmask_src = (sx & 1) ? 0x0F : 0xF0;
#if kPocoPixelFormat == kCommodettoBitmapCLUT16
	buildColorMap((uint32_t *)((uint8_t *)bits->pixels) - 16, poco->clut + 32, ((RenderMaskedBits)pc)->remap);
#endif
#endif

	maskBits = ((const uint8_t *)mask->pixels) + (((mask->width + 1) >> 1) * mask_sy);  // clipped off top. skip sy scan lines.
	maskBits += (mask_sx >> 1);

	((RenderMaskedBits)pc)->maskBits = maskBits;
#if 1
	((RenderMaskedBits)pc)->maskBump = (mask->width + 1) >> 1;
	((RenderMaskedBits)pc)->mask = (sx & 1) ? 0 : 4;
#else
	((RenderMaskedBits)pc)->maskBump = (mask->width >> 1) - ((((mask_sx & 1) + sw) + 1) >> 1);
	((RenderMaskedBits)pc)->mask = (mask_sx & 1) ? 0x0F : 0xF0;
#endif

	((RenderMaskedBits)pc)->blendersOffset = blend & 0xF0;		// (blend >> 4) << 4

	PocoCommandBuilt(poco, pc);
}

void PocoBitmapPattern(Poco poco, PocoBitmap bits, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoDimension sx, PocoDimension sy, PocoDimension sw, PocoDimension sh)
{
	PocoCoordinate xMax, yMax;
	PocoCommand pc = poco->next;
	PatternBits pb;

	PocoReturnIfNoSpace(pc, sizeof(PatternBitsRecord));

	rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);
#if (0 == kPocoRotation) || (180 == kPocoRotation)
	rotateCoordinatesAndDimensions(bits->width, bits->height, sx, sy, sw, sh);
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
	rotateCoordinatesAndDimensions(bits->height, bits->width, sx, sy, sw, sh);
#endif

	pc->command = kPocoCommandBitmapPattern;
	PocoCommandSetLength(pc, sizeof(PatternBitsRecord));
	pb = (PatternBits)pc;
	pb->xOffset = 0;
	pb->dy = 0;
	pb->patternW = sw;
	pb->patternH = sh;

	xMax = x + w;
	yMax = y + h;

	if (x < poco->x) {
		pb->xOffset = (poco->x - x) % sw;
		x = poco->x;
	}

	if (xMax > poco->xMax)
		xMax = poco->xMax;

	if (x >= xMax)
		return;

	w = xMax - x;

	if (y < poco->y) {
		pb->dy = (poco->y - y) % sh;
		y = poco->y;
	}

	if (yMax > poco->yMax)
		yMax = poco->yMax;

	if (y >= yMax)
		return;

	h = yMax - y;

	pc->x = x, pc->y = y, pc->w = w, pc->h = h;

#if 4 != kPocoPixelSize
	pb->patternStart = ((PocoPixel *)bits->pixels) + (sy * bits->width) + (sx + pb->xOffset);
	pb->pixels = pb->patternStart + (pb->dy * bits->width);
	pb->rowBump = bits->width + pb->xOffset;
#else
	pb->patternStart = ((PocoPixel *)bits->pixels) + (((sy * bits->width) + (sx + pb->xOffset)) >> 1);
	pb->pixels = pb->patternStart + ((pb->dy * bits->width) >> 1);
	pb->rowBump = (bits->width + pb->xOffset) >> 1;

#if kPocoPixelFormat == kCommodettoBitmapCLUT16
	buildColorMap((uint32_t *)((uint8_t *)bits->pixels) - 16, poco->clut + 32, pb->remap);
#endif
#endif

	PocoCommandBuilt(poco, pc);
}

void PocoDrawFrame(Poco poco, uint8_t *data, uint32_t dataSize, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PocoCommand pc = poco->next;
	Frame f;
	PocoCoordinate xMax, yMax, clipRight;
#if (0 == kPocoRotation) || (180 == kPocoRotation)
	PocoCoordinate sw = w;
#else
	PocoCoordinate sw = h;
#endif

	PocoReturnIfNoSpace(pc, sizeof(FrameRecord));

	rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);

	pc->command = kPocoCommandFrame;
	PocoCommandSetLength(pc, sizeof(FrameRecord));
	f = (Frame)pc;
	f->data = data;
	f->yOffset = 0;
	f->blockWidth = sw >> 2;
	f->prev0 = PocoMakeColor(poco, 255, 255, 255);
	f->prev1 = PocoMakeColor(poco, 0, 0, 0);
	f->clipLeft = 0;
	clipRight = 0;

	xMax = x + w;
	yMax = y + h;

	if (xMax > poco->xMax) {
		clipRight = xMax - poco->xMax;
		xMax = poco->xMax;
	}

	if (x >= xMax)
		return;

	if (x < poco->x) {
		f->clipLeft = poco->x - x;
		x = poco->x;
	}

	if (yMax > poco->yMax)
		yMax = poco->yMax;

	if (y >= yMax)
		return;

	if (y < poco->y) {
		doSkipColorCells(poco, pc, ((poco->y - y) >> 2) * f->blockWidth);
		f->yOffset = (poco->y - y) & 3;
		y = poco->y;
	}

	f->unclippedBlockWidth = ((sw - clipRight + 3) >> 2) - (f->clipLeft >> 2);

	pc->x = x, pc->y = y, pc->w = xMax - x, pc->h = yMax - y;

	PocoCommandBuilt(poco, pc);
}

// rotation and clipped performed externally
void PocoDrawExternal(Poco poco, PocoRenderExternal doDrawExternal, uint8_t *data, uint8_t dataSize, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PocoCommand pc = poco->next;
	External e;

	PocoReturnIfNoSpace(pc, sizeof(ExternalRecord) + dataSize - 1);

	pc->command = kPocoCommandExternal;
	PocoCommandSetLength(pc, sizeof(ExternalRecord) + dataSize - 1);
	e = (External)pc;
	e->doDrawExternal = doDrawExternal;
	c_memcpy(e->data, data, dataSize);

	pc->x = x, pc->y = y, pc->w = w, pc->h = h;

	PocoCommandBuilt(poco, pc);
}

/*
	BMFont support
*/

int PocoNextFromUTF8(uint8_t **src)
{
	int result;
	uint8_t *s = *src;
	uint8_t c = c_read8(s++);

	if (!(c & 0x80)) {
		*src = s;
		return c;
	}

	if (0xC0 == (c & 0xE0)) { // 2 byte sequence
		result = ((c & 0x1F) << 6) | (c_read8(s) & 0x3F);
		s += 1;
	}
	else if (0xE0 == (c & 0xF0)) { // 3 byte sequence
		result = ((c & 0x0F) << 12) | ((c_read8(s) & 0x3F) << 6) | (c_read8(s + 1) & 0x3F);
		s += 2;
	}
	else if (0xF0 == (c & 0xF1)) { // 4 byte sequence
		result = ((c & 0x07) << 18) | ((c_read8(s) & 0x3F) << 12) | ((c_read8(s + 1) & 0x3F) << 6) | (c_read8(s + 2) & 0x3F);
		s += 3;
	}
	else
		result = 0;

	*src = s;
	return result;
}

const uint8_t *PocoBMFGlyphFromUTF8(uint8_t **src, const uint8_t *chars, int charCount)
{
	int min, max;
	int c = PocoNextFromUTF8(src);
	if (!c)
		return NULL;

	if (c_read8(chars + 19) & 0x40) {
		// one run of continuously numbered characters
		int firstChar = c_read32(chars);
		if (c < firstChar)
			return NULL;

		c -= firstChar;
		if (c >= charCount)
			return NULL;

		return (20 * c) + chars;
	}

	// ascending order, with gaps. binary search.
	min = 0;
	max = charCount;
	do {
		int mid = (min + max) >> 1;
		const uint8_t *cc = (20 * mid) + chars;
		int code = c_read32(cc);
		if (code < c)
			min = mid + 1;
		else if (c < code)
			max = mid - 1;
		else
			return cc;
	} while (min <= max);

	return NULL;
}

/*
	here begin the functions to render the drawing list
*/

void doFillRectangle(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	PocoCoordinate w = pc->w;
#if 4 != kPocoPixelSize
	PocoCoordinate rowBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - w;
#endif
	PocoPixel color = ((ColorDraw)pc)->color;

#if 16 == kPocoPixelSize
	uint32_t colors = ((uint32_t)color << 16) | color;

	while (h--) {
		PocoCoordinate tw = w;

		if (2 & (uintptr_t)dst) {
			*dst++ = (PocoPixel)colors;
			tw -=1;
		}

		while (tw >= 16) {
			*(uint32_t *)&dst[0] = colors;
			*(uint32_t *)&dst[2] = colors;
			*(uint32_t *)&dst[4] = colors;
			*(uint32_t *)&dst[6] = colors;
			*(uint32_t *)&dst[8] = colors;
			*(uint32_t *)&dst[10] = colors;
			*(uint32_t *)&dst[12] = colors;
			*(uint32_t *)&dst[14] = colors;
			tw -= 16;
			dst += 16;
		}

		while (tw >= 4) {
			*(uint32_t *)&dst[0] = colors;
			*(uint32_t *)&dst[2] = colors;
			tw -= 4;
			dst += 4;
		}

		if (0 == tw)
			;
		else if (1 == tw)
			*dst = (PocoPixel)colors;
		else if (2 == tw)
			*(uint32_t *)&dst[0] = colors;
		else {
			*(uint32_t *)&dst[0] = colors;
			*(uint16_t *)&dst[2] = (PocoPixel)colors;
		}

		dst += rowBump + tw;
	}
#elif 4 == kPocoPixelSize
	const uint8_t colors = color | (color << 4);
	const uint8_t xphase = pc->xphase;

	while (h--) {
		PocoPixel *dNext = (PocoPixel *)(poco->rowBytes + (char *)dst);
		PocoCoordinate tw = w;

		if (xphase) {
			uint8_t d = *dst;
			*dst++ = (d & kPocoPixels4FirstMask) | (color << kPocoPixels4SecondShift);

			tw -= 1;
		}

		while (tw >= 8) {
			*dst++ = colors;
			*dst++ = colors;
			*dst++ = colors;
			*dst++ = colors;
			tw -= 8;
		}

		while (tw >= 2) {
			*dst++ = colors;
			tw -= 2;
		}

		if (tw) {
			uint8_t d = *dst;
			*dst++ = (d & kPocoPixels4SecondMask) | (color << kPocoPixels4FirstShift);
		}

		dst = dNext;
	}
#else
	if (w >= 24) {
		uint32_t colors = (color << 8) | color;
		colors |= (colors << 16);
		while (h--) {
			uint8_t t = 3 & (uintptr_t)dst;
			PocoCoordinate tw = w;

			if (t) {
				t = 4 - t;
				tw -= t;
				while (t--)
					*dst++ = color;
			}

			while (tw >= 16) {
				((uint32_t *)dst)[0] = colors;
				((uint32_t *)dst)[1] = colors;
				((uint32_t *)dst)[2] = colors;
				((uint32_t *)dst)[3] = colors;
				dst += 16;
				tw -= 16;
			}

			while (tw >= 4) {
				*(uint32_t *)dst = colors;
				dst += 4;
				tw -= 4;
			}

			while (tw--)
				*dst++ = color;
			dst += rowBump;
		}
	}
	else {
		while (h--) {
			PocoCoordinate tw = w;

			while (tw >= 4) {
				*dst++ = color;
				*dst++ = color;
				*dst++ = color;
				*dst++ = color;
				tw -= 4;
			}
			while (tw--)
				*dst++ = color;
			dst += rowBump;
		}
	}
#endif
}

#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
// based on FskFastBlend565SE - one multiply per pixel
void doBlendRectangle(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	BlendDraw bd = (BlendDraw)pc;
	PocoCoordinate w = bd->w;
	PocoCoordinate rowBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - w;
	uint8_t blend = bd->blend;		// 5 bit blend level
	int src32;

	src32 = bd->color;
	src32 |= src32 << 16;
	src32 &= 0x07E0F81F;

	while (h--) {
		PocoCoordinate tw = w;

		while (tw--) {
			int	dst, src;

			dst = *d;
			dst |= dst << 16;
			dst &= 0x07E0F81F;
			src = src32 - dst;
			dst = blend * src + (dst << 5) - dst;
			dst += 0x02008010;
			dst += (dst >> 5) & 0x07E0F81F;
			dst >>= 5;
			dst &= 0x07E0F81F;
			dst |= dst >> 16;
			*d++ = (PocoPixel)dst;
		}
		d += rowBump;
	}
}
#elif kPocoPixelFormat == kCommodettoBitmapGray256
void doBlendRectangle(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	BlendDraw bd = (BlendDraw)pc;
	uint8_t blend = bd->blend;		// 5 bit blend level
	PocoCoordinate w = bd->w;
	PocoCoordinate rowBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - w;
	uint16_t srcColor = bd->color * blend;
	blend = 31 - blend;

	while (h--) {
		PocoCoordinate tw = w;

		while (tw--) {
			uint16_t d = *dst;
			*dst++ = ((d * blend) + srcColor) >> 5;
		}
		dst += rowBump;
	}
}
#elif kPocoPixelFormat == kCommodettoBitmapGray16
void doBlendRectangle(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	BlendDraw bd = (BlendDraw)pc;
	PocoCoordinate w = bd->w;
	PocoCoordinate rowBump = poco->rowBytes - ((bd->w + 1 + bd->xphase) >> 1);
	uint8_t blend = bd->blend;		// 5 bit blend level
	uint16_t srcColor = bd->color * blend;
	blend = 31 - blend;

	while (h--) {
		PocoCoordinate tw = w;
		uint8_t xphase = bd->xphase;

		while (tw--) {
			uint16_t d = *dst;
			uint8_t blended;
			if (xphase) {
				blended = (((d & 0x0F) * blend) + srcColor) >> 5;
				*dst++ = (d & 0xF0) | blended;
				xphase = 0;
			}
			else {
				blended = (((d >> 4) * blend) + srcColor) >> 5;
				*dst = (d & 0x0F) | (blended << 4);
				xphase = 1;
			}
		}

		dst += rowBump;
	}
}
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
void doBlendRectangle(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	BlendDraw bd = (BlendDraw)pc;
	PocoCoordinate w = bd->w;
	PocoCoordinate rowBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - w;
	uint8_t blend = bd->blend >> 2;		// 3 bit blend level
	int src32;

	src32 = bd->color;
	src32 |= src32 << 16;
	src32 &= 0x001C00E3;

	while (h--) {
		PocoCoordinate tw = w;

		while (tw--) {
			int	dst, src;

			dst = *d;
			dst |= dst << 16;
			dst &= 0x001C00E3;
			src = src32 - dst;
			dst = blend * (src32 - dst) + (dst << 3) - dst;
//@@ half bit precision			dst += 0x02008010;
			dst += (dst >> 3) & 0x000C00E3;
			dst >>= 3;
			dst &= 0x001C00E3;
			dst |= dst >> 16;
			*d++ = (PocoPixel)dst;
		}
		d += rowBump;
	}
}
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
void doBlendRectangle(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	BlendDraw bd = (BlendDraw)pc;
	int src32;
	uint16_t *clut = (uint16_t *)poco->clut;
	uint8_t *map = 32 + (uint8_t *)poco->clut;
	uint8_t blend = 15 - (bd->blend >> 1);		// 4 bit blend level
	PocoCoordinate rowBump = poco->rowBytes - ((bd->w + 1 + bd->xphase) >> 1);
	src32 = c_read16(clut + bd->color);
	src32 |= src32 << 16;
	src32 &= 0x00F00F0F;

	while (h--) {
		PocoCoordinate tw = bd->w;
		uint8_t xphase = bd->xphase;

		while (tw--) {
			uint8_t dstTwo = *dst;
			if (xphase) {
#if kPocoCLUT16_01
				int ds = c_read16(clut + (dstTwo & 0x0F));
#else
				int ds = c_read16(clut + (dstTwo >> 4));
#endif
				ds |= ds << 16;
				ds &= 0x00F00F0F;
				ds = blend * (src32 - ds) + (ds << 4) - ds;
				ds += (ds >> 4) & 0x00F00F0F;
				ds >>= 4;
				ds &= 0x00F00F0F;
				ds |= ds >> 16;

				*dst++ = (dstTwo & kPocoPixels4FirstMask) | (c_read8(map + (uint16_t)ds) << kPocoPixels4SecondShift);
				xphase = 0;
			}
			else {
#if kPocoCLUT16_01
				int ds = c_read16(clut + (dstTwo >> 4));
#else
				int ds = c_read16(clut + (dstTwo & 0x0F));
#endif
				ds |= ds << 16;
				ds &= 0x00F00F0F;
				ds = blend * (src32 - ds) + (ds << 4) - ds;
				ds += (ds >> 4) & 0x00F00F0F;
				ds >>= 4;
				ds &= 0x00F00F0F;
				ds |= ds >> 16;

				*dst = (dstTwo & kPocoPixels4SecondMask) | (c_read8(map + (uint16_t)ds) << kPocoPixels4FirstShift);
				xphase = 1;
			}
		}

		dst += rowBump;
	}
}
#else
	#error
#endif

#if kPocoPixelSize >= 8

void doDrawPixel(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
   *dst = ((ColorDraw)pc)->color;
}

#elif 4 == kPocoPixelSize

void doDrawPixel(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	uint8_t d = *dst;

	if (PocoPixels4IsSecondPixel(pc->xphase))
		*dst = ((ColorDraw)pc)->color | (d & 0xF0);
	else
		*dst = ((((ColorDraw)pc)->color) << 4) | (d & 0x0F);
}

#endif

#if 4 != kPocoPixelSize
// maybe variations of this function for small / large widths (or alignment?) to eliminate checks on each invocation
void doDrawBitmap(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	RenderBits srcBits = (RenderBits)pc;
	int count = srcBits->w * sizeof(PocoPixel);
	PocoPixel *src = srcBits->pixels;
	PocoCoordinate srcRowPixels = srcBits->rowPixels;

	while (h--) {
		c_memcpy(dst, src, count);	// inline could be faster? (seems no)
		dst = (PocoPixel *)(poco->rowBytes + (char *)dst);
		src += srcRowPixels;
	}
	srcBits->pixels = src;
}

#else

void doDrawBitmap(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	RenderBits srcBits = (RenderBits)pc;
	PocoPixel *src = srcBits->pixels;
	PocoCoordinate srcRowBytes = srcBits->rowBytes;
	int dstRowBytes = poco->rowBytes;

	while (h--) {
#if kPocoPixelFormat == kCommodettoBitmapGray16
		(srcBits->doCopy)(dst, src, srcBits->w);
#else
		(srcBits->doCopy)(dst, src, srcBits->w, srcBits->remap);
#endif
		dst = (PocoPixel *)(dstRowBytes  + (char *)dst);
		src += srcRowBytes;
	}
	srcBits->pixels = src;
}

#endif

void doDrawMonochromeBitmapPart(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	RenderMonochromeBits srcBits = (RenderMonochromeBits)pc;
	const uint8_t *src = srcBits->pixels;
	uint8_t mask = srcBits->mask;
#if 4 != kPocoPixelSize
	PocoCoordinate scanBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - srcBits->w;
#endif
	PocoPixel fore = srcBits->fore, back = srcBits->back, drawFore = (srcBits->mode & kPocoMonochromeForeground) != 0, drawBack = (srcBits->mode & kPocoMonochromeBackground) != 0;

	while (h--) {
#if 4 == kPocoPixelSize
		PocoPixel *dstNext = (PocoPixel *)(poco->rowBytes + (char *)dst);
#endif
		PocoCoordinate tw = srcBits->w;
		uint32_t *srcLong = (uint32_t *)(~3 & (uintptr_t)src);
		uint32_t bits = *srcLong++;
		uint32_t tm = mask << (24 - ((3 & (uintptr_t)src) << 3));
#if 4 == kPocoPixelSize
		uint8_t xphase = srcBits->xphase;
#endif
		bits = SwapLong(bits);

		while (tw--) {
			if (0 == tm) {
				tm = 0x80000000;
				bits = *srcLong++;
				bits = SwapLong(bits);
			}

#if 4 != kPocoPixelSize
			if (bits & tm) {
				if (drawFore)
					*dst = fore;
			}
			else
			if (drawBack)
				*dst = back;
			dst += 1;
#else
			if (bits & tm) {
				if (drawFore) {
					if (PocoPixels4IsSecondPixel(xphase))
						*dst = (*dst & 0xF0) | fore;
					else
						*dst = (fore << 4) | (*dst & 0x0F);
				}
			}
			else
			if (drawBack) {
				if (PocoPixels4IsSecondPixel(xphase))
					*dst = (*dst & 0xF0) | back;
				else
					*dst = (back << 4) | (*dst & 0x0F);
			}
			dst += xphase;
			xphase ^= 1;
#endif

			tm >>= 1;
		}

		src += srcBits->rowBump;
#if 4 != kPocoPixelSize
		dst += scanBump;
#else
		dst = dstNext;
#endif
	}

	srcBits->pixels = src;
}

void doDrawMonochromeForegroundBitmapPart(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	RenderMonochromeBits srcBits = (RenderMonochromeBits)pc;
	const uint8_t *src = srcBits->pixels;
	uint8_t mask = srcBits->mask;
#if 4 != kPocoPixelSize
	PocoCoordinate scanBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - srcBits->w;
#endif
	PocoPixel fore = srcBits->fore;

	while (h--) {
#if 4 == kPocoPixelSize
		PocoPixel *dstNext = (PocoPixel *)(poco->rowBytes + (char *)dst);
#endif
		PocoCoordinate tw = srcBits->w;
		uint32_t *srcLong = (uint32_t *)(~3 & (uintptr_t)src);
		uint32_t bits = *srcLong++;
		uint32_t tm = mask << (24 - ((3 & (uintptr_t)src) << 3));

#if 4 == kPocoPixelSize
		uint8_t xphase = srcBits->xphase;
#endif
		bits = SwapLong(bits);

		while (tw--) {
			if (0 == tm) {
				tm = 0x80000000;
				bits = *srcLong++;
				bits = SwapLong(bits);
			}

#if 4 != kPocoPixelSize
			if (bits & tm)
				*dst = fore;
			dst += 1;
#else
			if (bits & tm) {
				if (PocoPixels4IsSecondPixel(xphase))
					*dst = (*dst & 0xF0) | fore;
				else
					*dst = (fore << 4) | (*dst & 0x0F);
			}
			dst += xphase;
			xphase ^= 1;
#endif
			tm >>= 1;
		}

		src += srcBits->rowBump;
#if 4 != kPocoPixelSize
		dst += scanBump;
#else
		dst = dstNext;
#endif
	}

	srcBits->pixels = src;
}

void doDrawGray16BitmapPart(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	RenderGray16Bits srcBits = (RenderGray16Bits)pc;
	const uint8_t *src = srcBits->pixels;
	uint8_t mask = srcBits->mask;
#if 4 != kPocoPixelSize
	PocoCoordinate scanBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - srcBits->w;
#endif
	PocoColor color = srcBits->color;
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
	uint32_t src32 = (color | ((uint32_t)color << 16)) & 0x07E0F81F;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
	uint32_t src32 = (color | (color << 16)) & 0x001C00E3;
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
	uint32_t src32 = (color | (color << 16)) & 0x00F00F0F;
	uint16_t *clut = (uint16_t *)poco->clut;
	uint8_t *map = 32 + (uint8_t *)poco->clut;
	color = c_read8(map + color);		// color index
#endif
#ifndef __ets__
	const uint8_t *blender = gBlenders + srcBits->blendersOffset;
#else
	uint8_t blender[16];
	uint32_t *bfrom = (uint32_t *)(gBlenders + srcBits->blendersOffset), *bto = (uint32_t *)blender;
	bto[0] = bfrom[0];
	bto[1] = bfrom[1];
	bto[2] = bfrom[2];
	bto[3] = bfrom[3];
#endif


	while (h--) {
#if 4 == kPocoPixelSize
		PocoPixel *dNext = (PocoPixel *)(poco->rowBytes + (char *)d);
#endif
		PocoCoordinate tw = srcBits->w;
		uint32_t *srcLong = (uint32_t *)(~3 & (uintptr_t)src);
		uint32_t bits = *srcLong++;		// 32 bits of mask - 8 4-bit gray values
		uint8_t tm = mask + ((3 - (3 & (uintptr_t)src)) << 3);
#if 4 == kPocoPixelSize
		uint8_t xphase = srcBits->xphase;
#endif

		bits = SwapLong(bits);

		while (tw--) {
			uint8_t blend;

			if (((int8_t)tm) < 0) {
				tm = 32 - 4;
				bits = *srcLong++;
				bits = SwapLong(bits);
			}

			blend = *(blender + ((bits >> tm) & 0x0f));		// 5-bit blend above 8-bit, 4 bit below
			if (blend) {
				if (31 == blend) {
#if 4 != kPocoPixelSize
					*d = color;
#else
					if (PocoPixels4IsSecondPixel(xphase))
						*d = (*d & 0xF0) | color;
					else
						*d = (color << 4) | (*d & 0x0F);
#endif
				}
				else {
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
					uint32_t dst = *d;

					dst |= dst << 16;
					dst &= 0x07E0F81F;
					dst = blend * (src32 - dst) + (dst << 5) - dst;
					dst += 0x02008010;
					dst += (dst >> 5) & 0x07E0F81F;
					dst >>= 5;
					dst &= 0x07E0F81F;
					dst |= dst >> 16;
					*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray256
					*d = ((*d * (15 - blend)) + (color * blend)) >> 4;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
					uint32_t dst = *d;

					blend = blend >> 1;		// 3-bit blend

					dst |= dst << 16;
					dst &= 0x001C00E3;
					dst = blend * (src32 - dst) + (dst << 3) - dst;
//@@ half bit precision			dst += 0x02008010;
					dst += (dst >> 3) & 0x000C00E3;
					dst >>= 3;
					dst &= 0x001C00E3;
					dst |= dst >> 16;
					*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray16
					PocoPixel pixels = *d;
					if (xphase)
						*d = (pixels & 0xF0) | ((((pixels & 0x0F) * (15 - blend)) + (color * blend)) >> 4);
					else
						*d = ((((pixels >> 4) * (15 - blend)) + (color * blend)) & 0xF0) | (pixels & 0x0F);
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
					uint8_t dstTwo = *d;
					if (PocoPixels4IsSecondPixel(xphase)) {
						uint32_t ds = c_read16(clut + (dstTwo & 0x0F));
						ds |= ds << 16;
						ds &= 0x00F00F0F;
						ds = blend * (src32 - ds) + (ds << 4) - ds;
						ds += (ds >> 4) & 0x00F00F0F;
						ds >>= 4;
						ds &= 0x00F00F0F;
						ds |= ds >> 16;

						*d = (dstTwo & 0xF0) | c_read8(map + (uint16_t)ds);
					}
					else {
						uint32_t ds = c_read16(clut + (dstTwo >> 4));
						ds |= ds << 16;
						ds &= 0x00F00F0F;
						ds = blend * (src32 - ds) + (ds << 4) - ds;
						ds += (ds >> 4) & 0x00F00F0F;
						ds >>= 4;
						ds &= 0x00F00F0F;
						ds |= ds >> 16;

						*d = (dstTwo & 0x0F) | (c_read8(map + (uint16_t)ds) << 4);
					}
#else
	#error
#endif
				}
			}
#if 4 != kPocoPixelSize
			d += 1;
#else
			d += xphase;
			xphase ^= 1;
#endif
			tm -= 4;
		}

		src += srcBits->rowBump;
#if 4 != kPocoPixelSize
		d += scanBump;
#else
		d = dNext;
#endif
	}

	srcBits->pixels = src;
}

void doDrawGray16RLEBitmapPart(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	RenderGray16RLEBits srcBits = (RenderGray16RLEBits)pc;
	PocoCoordinate skip = (PocoCommandLength(RenderGray16RLEBitsRecord) == pc->length) ? srcBits->skip : 0;
	const uint32_t *pixels = (const uint32_t *)srcBits->pixels;
	uint32_t nybbles = pixels[-1];
	uint8_t nybbleCount = srcBits->nybbleCount;
#if 4 != kPocoPixelSize
	PocoCoordinate scanBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - srcBits->w;
#endif
	PocoColor color = srcBits->color;
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
	uint32_t src32 = (color | ((uint32_t)color << 16)) & 0x07E0F81F;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
	uint32_t src32 = (color | (color << 16)) & 0x001C00E3;
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
	uint32_t src32 = (color | (color << 16)) & 0x00F00F0F;
	uint16_t *clut = (uint16_t *)poco->clut;
	uint8_t *map = 32 + (uint8_t *)poco->clut;
	color = c_read8(map + color);		// color index
#endif

	nybbles >>= (32 - (nybbleCount << 2));

	while (h--) {
		uint8_t nybble, count;
#if 4 == kPocoPixelSize
		PocoPixel *dNext = (PocoPixel *)(poco->rowBytes + (char *)d);
#endif
		PocoCoordinate remaining;
#if 4 == kPocoPixelSize
		uint8_t xphase = srcBits->xphase;
#endif

		/*
			clip left and previous right
		*/
		if (skip && srcBits->thisSkip) {
			remaining = srcBits->thisSkip;

			do {
				if (0 == nybbleCount) {
					nybbles = *pixels++;
					nybbleCount = 8;
				}

				nybble = nybbles & 0x0F;
				nybbles >>= 4;
				nybbleCount -= 1;

				if (8 & nybble) {
					if (0x08 == (nybble & 0x0C)) {		// solid
						count = (nybble & 3) + 2;
						if (count > remaining) {
							count -= remaining;
							remaining = srcBits->w;
							goto blitSolid;
						}
						remaining -= count;
					}
					else {		// quote
						count = (nybble & 3) + 1;
						do {
							if (0 == nybbleCount) {
								nybbles = *pixels++;
								nybbleCount = 8;
							}

							nybbles >>= 4;
							nybbleCount -= 1;
							remaining -= 1;
							count -= 1;
						} while (remaining && count);

						if (count) {
							remaining = srcBits->w;
							goto blitQuote;
						}
					}
				}
				else {	// skip
					count = nybble + 2;
					if (count > remaining) {
						count -= remaining;
						remaining = srcBits->w;
						goto blitSkip;
					}
					remaining -= count;
				}
			} while (remaining);
		}

		/*
			draw
		*/
		remaining = srcBits->w;
		while (remaining > 0) {
			if (0 == nybbleCount) {
				nybbles = *pixels++;
				nybbleCount = 8;
			}

			nybble = nybbles & 0x0F;
			nybbles >>= 4;
			nybbleCount -= 1;

			if (8 & nybble) {
				if (0x08 == (nybble & 0x0C)) {
					count = (nybble & 3) + 2;	// solid

blitSolid:
					remaining -= count;
					if (remaining < 0)
						count += remaining;

					while (count--) {
#if 4 != kPocoPixelSize
						*d++ = color;
#else
						if (PocoPixels4IsSecondPixel(xphase)) {
							*d = (*d & 0xF0) | color;
							d += 1;
						}
						else
							*d = (color << 4) | (*d & 0x0F);
						xphase ^= 1;
#endif
					}
				}
				else {		// quote
					count = (nybble & 3) + 1;

blitQuote:
					remaining -= count;
					if (remaining < 0)
						count += remaining;

					while (count--) {
						uint8_t blend;

						if (0 == nybbleCount) {
							nybbles = *pixels++;
							nybbleCount = 8;
						}

						blend = 15 - (nybbles & 0x0F);
						nybbles >>= 4;
						nybbleCount -= 1;

						if (blend) {
							if (15 == blend) {
#if 4 != kPocoPixelSize
								*d = color;
#else
								if (PocoPixels4IsSecondPixel(xphase))
									*d = (*d & 0xF0) | color;
								else
									*d = (color << 4) | (*d & 0x0F);
#endif
							}
							else {
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
								uint32_t dst = *d;

								blend = (blend << 1) | (blend >> 3);		// 5-bit blend

								dst |= dst << 16;
								dst &= 0x07E0F81F;
								dst = blend * (src32 - dst) + (dst << 5) - dst;
								dst += 0x02008010;
								dst += (dst >> 5) & 0x07E0F81F;
								dst >>= 5;
								dst &= 0x07E0F81F;
								dst |= dst >> 16;
								*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray256
								*d = ((*d * (15 - blend)) + (color * blend)) >> 4;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
								uint32_t dst = *d;

								blend = blend >> 1;		// 3-bit blend

								dst |= dst << 16;
								dst &= 0x001C00E3;
								dst = blend * (src32 - dst) + (dst << 3) - dst;
			//@@ half bit precision			dst += 0x02008010;
								dst += (dst >> 3) & 0x000C00E3;
								dst >>= 3;
								dst &= 0x001C00E3;
								dst |= dst >> 16;
								*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray16
								PocoPixel pixels = *d;
								if (xphase)
									*d = (pixels & 0xF0) | ((((pixels & 0x0F) * (15 - blend)) + (color * blend)) >> 4);
								else
									*d = ((((pixels >> 4) * (15 - blend)) + (color * blend)) & 0xF0) | (pixels & 0x0F);
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
								uint8_t dstTwo = *d;
								if (PocoPixels4IsSecondPixel(xphase)) {
									uint32_t ds = c_read16(clut + (dstTwo & 0x0F));
									ds |= ds << 16;
									ds &= 0x00F00F0F;
									ds = blend * (src32 - ds) + (ds << 4) - ds;
									ds += (ds >> 4) & 0x00F00F0F;
									ds >>= 4;
									ds &= 0x00F00F0F;
									ds |= ds >> 16;

									*d = (dstTwo & 0xF0) | c_read8(map + (uint16_t)ds);
								}
								else {
									uint32_t ds = c_read16(clut + (dstTwo >> 4));
									ds |= ds << 16;
									ds &= 0x00F00F0F;
									ds = blend * (src32 - ds) + (ds << 4) - ds;
									ds += (ds >> 4) & 0x00F00F0F;
									ds >>= 4;
									ds &= 0x00F00F0F;
									ds |= ds >> 16;

									*d = (dstTwo & 0x0F) | (c_read8(map + (uint16_t)ds) << 4);
								}
#else
	#error
#endif
							}
						}
#if 4 != kPocoPixelSize
						d += 1;
#else
						d += xphase;
						xphase ^= 1;
#endif
					}

					if (remaining < 0) {
						count = -remaining;

						do {
							if (0 == nybbleCount) {
								nybbles = *pixels++;
								nybbleCount = 8;
							}

							nybbles >>= 4;
							nybbleCount -= 1;
						} while (--count);
					}
				}
			}
			else {		// skip
				count = nybble + 2;
blitSkip:
				remaining -= count;
				if (remaining < 0)
					count += remaining;

#if 4 != kPocoPixelSize
				d += count;
#else
				d += count >> 1;
				if (1 & count) {
					d += xphase;
					xphase ^= 1;
				}
#endif
			}
		}

		/*
			save clip right for next scan line
		*/
		if (skip)
			srcBits->thisSkip = skip + remaining;

#if 4 != kPocoPixelSize
		d += scanBump;
#else
		d = dNext;
#endif
	}

	srcBits->pixels = (uint8_t *)pixels;
	srcBits->nybbleCount = nybbleCount;
}

void doDrawGray16RLEBlendBitmapPart(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	RenderGray16RLEBits srcBits = (RenderGray16RLEBits)pc;
	PocoCoordinate skip = (PocoCommandLength(RenderGray16RLEBitsRecord) == pc->length) ? srcBits->skip : 0;
	const uint32_t *pixels = (const uint32_t *)srcBits->pixels;
	uint32_t nybbles = pixels[-1];
	uint8_t nybbleCount = srcBits->nybbleCount;
#if 4 != kPocoPixelSize
	PocoCoordinate scanBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - srcBits->w;
#endif
	PocoColor color = srcBits->color;
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
	uint32_t src32 = (color | (color << 16)) & 0x07E0F81F;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
	uint32_t src32 = (color | (color << 16)) & 0x001C00E3;
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
	uint32_t src32 = (color | (color << 16)) & 0x00F00F0F;
	uint16_t *clut = (uint16_t *)poco->clut;
	uint8_t *map = 32 + (uint8_t *)poco->clut;
	color = c_read8(map + color);		// color index
#endif
	const uint8_t *blender = gBlenders + srcBits->blendersOffset;

	nybbles >>= (32 - (nybbleCount << 2));

	while (h--) {
		uint8_t nybble, count;
#if 4 == kPocoPixelSize
		PocoPixel *dNext = (PocoPixel *)(poco->rowBytes + (char *)d);
#endif
		PocoCoordinate remaining;
#if 4 == kPocoPixelSize
		uint8_t xphase = srcBits->xphase;
#endif

		/*
		 clip left and previous right
		 */
		if (skip && srcBits->thisSkip) {
			remaining = srcBits->thisSkip;

			do {
				if (0 == nybbleCount) {
					nybbles = *pixels++;
					nybbleCount = 8;
				}

				nybble = nybbles & 0x0F;
				nybbles >>= 4;
				nybbleCount -= 1;

				if (8 & nybble) {
					if (0x08 == (nybble & 0x0C)) {		// solid
						count = (nybble & 3) + 2;
						if (count > remaining) {
							count -= remaining;
							remaining = srcBits->w;
							goto blitSolid;
						}
						remaining -= count;
					}
					else {		// quote
						count = (nybble & 3) + 1;
						do {
							if (0 == nybbleCount) {
								nybbles = *pixels++;
								nybbleCount = 8;
							}

							nybbles >>= 4;
							nybbleCount -= 1;
							remaining -= 1;
							count -= 1;
						} while (remaining && count);

						if (count) {
							remaining = srcBits->w;
							goto blitQuote;
						}
					}
				}
				else {	// skip
					count = nybble + 2;
					if (count > remaining) {
						count -= remaining;
						remaining = srcBits->w;
						goto blitSkip;
					}
					remaining -= count;
				}
			} while (remaining);
		}

		/*
		 draw
		 */
		remaining = srcBits->w;
		while (remaining > 0) {
			if (0 == nybbleCount) {
				nybbles = *pixels++;
				nybbleCount = 8;
			}

			nybble = nybbles & 0x0F;
			nybbles >>= 4;
			nybbleCount -= 1;

			if (8 & nybble) {
				uint8_t blend;

				if (0x08 == (nybble & 0x0C)) {
					count = (nybble & 3) + 2;	// solid

				blitSolid:
					blend = c_read8(&blender[0]);
					remaining -= count;
					if (remaining < 0)
						count += remaining;
					if (31 == blend) {
						while (count--) {
	#if 4 != kPocoPixelSize
							*d++ = color;
	#else
							if (PocoPixels4IsSecondPixel(xphase)) {
								*d = (*d & 0xF0) | color;
								d += 1;
							}
							else
								*d = (color << 4) | (*d & 0x0F);
							xphase ^= 1;
	#endif
						}
					}
					else if (blend) {
						BlendDrawRecord bd;

						bd.color = color;
						bd.blend = blend;
						bd.w = count;
						// don't need to set bd.rowBytes when h == 1
#if 4 == kPocoPixelSize
						bd.xphase = xphase;
#endif
						doBlendRectangle(poco, (PocoCommand)&bd, d, 1);
						goto blitSkip2;
					}
				}
				else {		// quote
					count = (nybble & 3) + 1;

				blitQuote:
					remaining -= count;
					if (remaining < 0)
						count += remaining;

					while (count--) {

						if (0 == nybbleCount) {
							nybbles = *pixels++;
							nybbleCount = 8;
						}

						blend = c_read8(&blender[nybbles & 0x0F]);
						nybbles >>= 4;
						nybbleCount -= 1;

						if (blend) {
							if (31 == blend) {
#if 4 != kPocoPixelSize
								*d = color;
#else
								if (PocoPixels4IsSecondPixel(xphase))
									*d = (*d & 0xF0) | color;
								else
									*d = (color << 4) | (*d & 0x0F);
#endif
							}
							else {
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
								uint32_t dst = *d;

//								blend = (blend << 1) | (blend >> 3);		// 5-bit blend

								dst |= dst << 16;
								dst &= 0x07E0F81F;
								dst = blend * (src32 - dst) + (dst << 5) - dst;
								dst += 0x02008010;
								dst += (dst >> 5) & 0x07E0F81F;
								dst >>= 5;
								dst &= 0x07E0F81F;
								dst |= dst >> 16;
								*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray256
								blend >>= 1;			// 4 bit blend
								*d = ((*d * (15 - blend)) + (color * blend)) >> 4;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
								uint32_t dst = *d;

								blend >>= 2;			// 3-bit blend

								dst |= dst << 16;
								dst &= 0x001C00E3;
								dst = blend * (src32 - dst) + (dst << 3) - dst;
								//@@ half bit precision			dst += 0x02008010;
								dst += (dst >> 3) & 0x000C00E3;
								dst >>= 3;
								dst &= 0x001C00E3;
								dst |= dst >> 16;
								*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray16
								PocoPixel pixels = *d;
								blend >>= 1;			// 4 bit blend
								if (xphase)
									*d = (pixels & 0xF0) | ((((pixels & 0x0F) * (15 - blend)) + (color * blend)) >> 4);
								else
									*d = ((((pixels >> 4) * (15 - blend)) + (color * blend)) & 0xF0) | (pixels & 0x0F);
#elif kPocoPixelFormat == kCommodettoBitmapCLUT16
								uint8_t dstTwo = *d;
								blend >>= 1;			// 4 bit blend
								if (PocoPixels4IsSecondPixel(xphase)) {
									uint32_t ds = c_read16(clut + (dstTwo & 0x0F));
									ds |= ds << 16;
									ds &= 0x00F00F0F;
									ds = blend * (src32 - ds) + (ds << 4) - ds;
									ds += (ds >> 4) & 0x00F00F0F;
									ds >>= 4;
									ds &= 0x00F00F0F;
									ds |= ds >> 16;

									*d = (dstTwo & 0xF0) | c_read8(map + (uint16_t)ds);
								}
								else {
									uint32_t ds = c_read16(clut + (dstTwo >> 4));
									ds |= ds << 16;
									ds &= 0x00F00F0F;
									ds = blend * (src32 - ds) + (ds << 4) - ds;
									ds += (ds >> 4) & 0x00F00F0F;
									ds >>= 4;
									ds &= 0x00F00F0F;
									ds |= ds >> 16;

									*d = (dstTwo & 0x0F) | (c_read8(map + (uint16_t)ds) << 4);
								}
#else
#error
#endif
							}
						}
#if 4 != kPocoPixelSize
						d += 1;
#else
						d += xphase;
						xphase ^= 1;
#endif
					}

					if (remaining < 0) {
						count = -remaining;

						do {
							if (0 == nybbleCount) {
								nybbles = *pixels++;
								nybbleCount = 8;
							}

							nybbles >>= 4;
							nybbleCount -= 1;
						} while (--count);
					}
				}
			}
			else {		// skip
				count = nybble + 2;
			blitSkip:
				remaining -= count;
				if (remaining < 0)
					count += remaining;

			blitSkip2:
#if 4 != kPocoPixelSize
				d += count;
#else
				d += count >> 1;
				if (1 & count) {
					d += xphase;
					xphase ^= 1;
				}
#endif
			}
		}

		/*
		 save clip right for next scan line
		 */
		if (skip)
			srcBits->thisSkip = skip + remaining;

#if 4 != kPocoPixelSize
		d += scanBump;
#else
		d = dNext;
#endif
	}

	srcBits->pixels = (uint8_t *)pixels;
	srcBits->nybbleCount = nybbleCount;
}

#if 4 != kPocoPixelSize

void doDrawMaskedBitmap(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	RenderMaskedBits rmb = (RenderMaskedBits)pc;
	const PocoPixel *src = rmb->pixels;
	const uint8_t *maskBits = rmb->maskBits;
	uint8_t mask = rmb->mask;
	PocoCoordinate scanBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - rmb->w;
#ifndef __ets__
	const uint8_t *blender = gBlenders + rmb->blendersOffset;
#else
	uint8_t blender[16];
	uint32_t *bfrom = (uint32_t *)(gBlenders + rmb->blendersOffset), *bto = (uint32_t *)blender;
	bto[0] = bfrom[0];
	bto[1] = bfrom[1];
	bto[2] = bfrom[2];
	bto[3] = bfrom[3];
#endif

	while (h--) {
		PocoCoordinate tw = rmb->w;
		uint32_t *maskBitsLong = (uint32_t *)(~3 & (uintptr_t)maskBits);
		uint32_t bits = *maskBitsLong++;
		int8_t tm = mask + ((3 - (3 & (uintptr_t)maskBits)) << 3);
		bits = SwapLong(bits);

		while (tw--) {
			uint8_t blend;

			if (tm < 0) {
				tm = 32 - 4;
				bits = *maskBitsLong++;
				bits = SwapLong(bits);
			}

			blend = (bits >> tm) & 0x0f;		// 4-bit alpha
			if (15 != blend) {
				PocoPixel srcPixel;
				uint32_t pixels = *(uint32_t *)(~3 & (uintptr_t)src);		//@@ ouch
#if kPocoPixelSize == 16
				srcPixel = (PocoPixel)((0 == (3 & (uintptr_t)src)) ? pixels : (pixels >> 16));
#elif kPocoPixelSize == 8
				srcPixel = (PocoPixel)(pixels >> ((3 & (uintptr_t)src) << 3));
#else
	#error
#endif

				blend = *(blender + blend);		// 5-bit blend
				if (31 == blend)
					*d = srcPixel;
				else {
#if kPocoPixelFormat == kCommodettoBitmapRGB565LE
					uint32_t src32, dst;

					src32 = srcPixel;
					src32 |= src32 << 16;
					src32 &= 0x07E0F81F;

					dst = *d;
					dst |= dst << 16;
					dst &= 0x07E0F81F;
					dst = blend * (src32 - dst) + (dst << 5) - dst;
					dst += 0x02008010u;
					dst += (dst >> 5) & 0x07E0F81F;
					dst >>= 5;
					dst &= 0x07E0F81F;
					dst |= dst >> 16;
					*d = (PocoPixel)dst;
#elif kPocoPixelFormat == kCommodettoBitmapGray256
					*d = ((*d * (31 - blend)) + (srcPixel * blend)) >> 5;
#elif kPocoPixelFormat == kCommodettoBitmapRGB332
					uint32_t src32;
					uint32_t dst;

					src32 = srcPixel;
					src32 |= src32 << 16;
					src32 &= 0x001C00E3;

					blend >>= 2;			// 3-bit blend

					dst = *d;
					dst |= dst << 16;
					dst &= 0x001C00E3;
					dst = blend * (src32 - dst) + (dst << 3) - dst;
//@@ half bit precision			dst += 0x02008010;
					dst += (dst >> 3) & 0x000C00E3;
					dst >>= 3;
					dst &= 0x001C00E3;
					dst |= dst >> 16;
					*d = (PocoPixel)dst;
#else
	#error
#endif
				}
			}
			src += 1;
			d += 1;

			tm -= 4;
		}

		src += rmb->rowBump;
		maskBits += rmb->maskBump;
		d += scanBump;
	}

	rmb->pixels = src;
	rmb->maskBits = maskBits;
}

#else

void doDrawMaskedBitmap(Poco poco, PocoCommand pc, PocoPixel *d, PocoDimension h)
{
	RenderMaskedBits rmb = (RenderMaskedBits)pc;
	const PocoPixel *src = rmb->pixels;
	const uint8_t *maskBits = rmb->maskBits;
#if kPocoPixelFormat == kCommodettoBitmapCLUT16
	uint8_t *remap = rmb->remap;
	uint16_t *clut = (uint16_t *)poco->clut;
	uint8_t *map = 32 + (uint8_t *)poco->clut;
#endif
#ifndef __ets__
	const uint8_t *blender = gBlenders + rmb->blendersOffset;
#else
	uint8_t blender[16];
	uint32_t *bfrom = (uint32_t *)(gBlenders + rmb->blendersOffset), *bto = (uint32_t *)blender;
	bto[0] = bfrom[0];
	bto[1] = bfrom[1];
	bto[2] = bfrom[2];
	bto[3] = bfrom[3];
#endif

	while (h--) {
		PocoPixel *dNext = (PocoPixel *)(poco->rowBytes + (char *)d);
		PocoCoordinate tw = rmb->w;
		uint32_t *maskBitsLong = (uint32_t *)(~3 & (uintptr_t)maskBits);
		uint32_t bits = *maskBitsLong++;
		int8_t tm = rmb->mask + ((3 - (3 & (uintptr_t)maskBits)) << 3);
		uint8_t xmask_dst = rmb->xphase;
		uint8_t xmask_src = (0x0F == rmb->xmask_src);
		bits = SwapLong(bits);

		while (tw--) {
			uint8_t blend;

			if (tm < 0) {
				tm = 32 - 4;
				bits = *maskBitsLong++;
				bits = SwapLong(bits);
			}

			blend = (bits >> tm) & 0x0f;		// 4-bit alpha
			if (15 != blend) {
				PocoPixel srcPixel;
				uint32_t pixels = *(uint32_t *)(~3 & (uintptr_t)src);		//@@ ouch - 4 bytes each pixel

				srcPixel = (PocoPixel)(pixels >> ((3 & (uintptr_t)src) << 3));
#if kPocoPixelFormat == kCommodettoBitmapCLUT16
				if (xmask_src)
					srcPixel = remap[srcPixel & 0x0F];
				else
					srcPixel = remap[srcPixel >> 4];
#else
				if (xmask_src)
					srcPixel &= 0x0F;
				else
					srcPixel >>= 4;
#endif

				blend = *(blender + blend);		// 4-bit blend
				if (15 == blend) {
					PocoPixel pixels = *d;
	#if kPocoCLUT16_01
					if (xmask_dst) {
						*d++ = (pixels & 0xF0) | srcPixel;
						xmask_dst = 0;
					}
					else {
						*d = (srcPixel << 4) | (pixels & 0x0F);
						xmask_dst = 1;
					}
	#else
					if (xmask_dst) {
						*d++ = (srcPixel << 4) | (pixels & 0x0F);
						xmask_dst = 0;
					}
					else {
						*d = (pixels & 0xF0) | srcPixel;
						xmask_dst = 1;
					}
	#endif
				}
				else {
					PocoPixel dstTwo = *d;

#if kPocoPixelFormat == kCommodettoBitmapGray16
					if (xmask_dst) {
						*d++ = (dstTwo & 0xF0) | ((((dstTwo & 0x0F) * (15 - blend)) + (srcPixel * blend)) >> 4);
						xmask_dst = 0;
					}
					else {
						*d = ((((dstTwo >> 4) * (15 - blend)) + (srcPixel * blend)) & 0xF0) | (dstTwo & 0x0F);
						xmask_dst = 1;
					}
#else
					uint32_t src32 = c_read16(clut + srcPixel);
					src32 = (src32 | (src32 << 16)) & 0x00F00F0F;
					if (xmask_dst) {
	#if kPocoCLUT16_01
						int ds = c_read16(clut + (dstTwo & 0x0F));
	#else
						int ds = c_read16(clut + (dstTwo >> 4));
	#endif
						ds |= ds << 16;
						ds &= 0x00F00F0F;
						ds = blend * (src32 - ds) + (ds << 4) - ds;
						ds += (ds >> 4) & 0x00F00F0F;
						ds >>= 4;
						ds &= 0x00F00F0F;
						ds |= ds >> 16;

	#if kPocoCLUT16_01
						*d++ = (dstTwo & 0xF0) | c_read8(map + (uint16_t)ds);
	#else
						*d++ = (dstTwo & 0x0F) | (c_read8(map + (uint16_t)ds) << 4);
	#endif
						xmask_dst = 0;
					}
					else {
	#if kPocoCLUT16_01
						int ds = c_read16(clut + (dstTwo >> 4));
	#else
						int ds = c_read16(clut + (dstTwo & 0x0F));
	#endif

						ds |= ds << 16;
						ds &= 0x00F00F0F;
						ds = blend * (src32 - ds) + (ds << 4) - ds;
						ds += (ds >> 4) & 0x00F00F0F;
						ds >>= 4;
						ds &= 0x00F00F0F;
						ds |= ds >> 16;

	#if kPocoCLUT16_01
						*d = (dstTwo & 0x0F) | (c_read8(map + (uint16_t)ds) << 4);
	#else
						*d = (dstTwo & 0xF0) | c_read8(map + (uint16_t)ds);
	#endif
						xmask_dst = 1;
					}
#endif
				}
			}
			else {
				d += xmask_dst;
				xmask_dst ^= 1;
			}

			src += xmask_src;
			xmask_src ^= 1;

			tm -= 4;
		}

		src += rmb->rowBump;
		maskBits += rmb->maskBump;
		d = dNext;
	}

	rmb->pixels = src;
	rmb->maskBits = maskBits;
}

#endif

#if 4 != kPocoPixelSize

void doDrawPattern(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	PatternBits pb = (PatternBits)pc;
	const PocoPixel *src = pb->pixels;

	while (h--) {
		PocoCoordinate w = pb->w;
		PocoPixel *d = dst;
		PocoCoordinate use;

		if (pb->xOffset) {
			use = pb->patternW - pb->xOffset;
			if (use > w) use = w;
			c_memcpy(d, src, use * sizeof(PocoPixel));
			src -= pb->xOffset;
			d += use;
			w -= use;
		}

		use = pb->patternW;
		while (w >= use) {
			c_memcpy(d, src, use * sizeof(PocoPixel));
			d += use;
			w -= use;
		}

		if (w)
			c_memcpy(d, src, w * sizeof(PocoPixel));

		src += pb->rowBump;

		pb->dy += 1;
		if (pb->dy == pb->patternH) {
			pb->dy = 0;
			src = pb->patternStart;
		}

		dst = (PocoPixel *)(poco->rowBytes + (char *)dst);
	}

	pb->pixels = src;
}

#else

void doDrawPattern(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	PatternBits pb = (PatternBits)pc;
	const uint8_t *src = (const uint8_t *)pb->pixels;

	while (h--) {
		PocoCoordinate w = pb->w;
		PocoPixel *d = dst;
		PocoCoordinate use;
		PocoCoordinate dx = pb->xphase;
		PocoCoordinate sx = pb->xOffset;

		if (sx) {
			use = pb->patternW - sx;
			if (use > w) use = w;

#if kPocoPixelFormat == kCommodettoBitmapGray16
			(getMemCpy4(sx, dx))(d, src, use);
#else
			(getMemCpy4(sx, dx))(d, src, use, pb->remap);
#endif
			src -= sx >> 1;
			d += use >> 1;

			w -= use;
			sx += use, dx += use;
		}

		use = pb->patternW;
		while (w >= use) {
#if kPocoPixelFormat == kCommodettoBitmapGray16
			(getMemCpy4(sx, dx))(d, src, use);
#else
			(getMemCpy4(sx, dx))(d, src, use, pb->remap);
#endif
			d += use >> 1;

			w -= use;
			sx += use, dx += use;
		}

		if (w) {
#if kPocoPixelFormat == kCommodettoBitmapGray16
			(getMemCpy4(sx, dx))(d, src, w);
#else
			(getMemCpy4(sx, dx))(d, src, w, pb->remap);
#endif
			d += w >> 1;
		}

		src += pb->rowBump;

		pb->dy += 1;
		if (pb->dy == pb->patternH) {
			pb->dy = 0;
			src = pb->patternStart;
		}

		dst = (PocoPixel *)(poco->rowBytes + (char *)dst);
	}

	pb->pixels = src;
}
#endif

#if 4 == kPocoPixelSize

#if kPocoPixelFormat == kCommodettoBitmapGray16

void copy4AlignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels)
{
	c_memcpy(to, from, pixels >> 1);

	if (1 & pixels) {
		uint8_t offset;
		uint32_t eightPixels;
		uint8_t pixel;

		from += pixels >> 1;
		to += pixels >> 1;

		offset = 3 & (uintptr_t)from;
		eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);

		pixel = eightPixels >> (((offset << 1) ^ 1) << 2);		// eightPixels pixels ordered: 6 7  4 5  2 3  0 1
		*to = (pixel << 4) | (*to & 0x0F);
	}
}

void copy4MisalignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels)
{
	uint8_t offset = 3 & (uintptr_t)from;
	uint32_t eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);
	uint8_t pixelsInLong = (8 - (offset << 1)) - 1;

	from += (4 - offset);

	while (pixels >= 2) {
		if (1 == pixelsInLong) {
			uint8_t twoPixels = (eightPixels >> 20) & 0xF0;

			eightPixels = *(uint32_t *)from;
			from += 4;
			pixelsInLong = 7;

			*to++ = twoPixels | ((eightPixels >> 4) & 0x0F);
		}
		else {
			uint16_t fourPixels = eightPixels >> ((7 - pixelsInLong) << 2);			// 7, 5, or 3 pixelsInLong
			*to++ = (fourPixels << 4) | (fourPixels >> 12);
			pixelsInLong -= 2;
		}

		pixels -= 2;
	}

	if (pixels) {		// pixelsInLong is at least 1
		uint8_t pixel = eightPixels >> (((8 - pixelsInLong) ^ 1) << 2);
		*to = (pixel << 4) | (*to & 0x0F);
	}
}

void copy4AlignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels)
{
	uint8_t offset = 3 & (uintptr_t)from;
	uint32_t eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);
	uint8_t pixelsInLong = (8 - (offset << 1));
	uint8_t pixel;

	from += (4 - offset);

	pixel = (eightPixels >> (((8 - pixelsInLong) ^ 1) << 2)) & 0x0F;
	*to = (*to & 0xF0) | pixel;
	to++;
	pixels -= 1;
	pixelsInLong -= 1;

	while (pixels >= 2) {
		if (1 == pixelsInLong) {
			uint8_t twoPixels = (eightPixels >> 20) & 0xF0;

			eightPixels = *(uint32_t *)from;
			from += 4;
			pixelsInLong = 7;

			*to++ = twoPixels | ((eightPixels >> 4) & 0x0F);
		}
		else {
			uint16_t fourPixels = eightPixels >> ((7 - pixelsInLong) << 2);			// 7, 5, or 3 pixelsInLong
			*to++ = (fourPixels << 4) | (fourPixels >> 12);
			pixelsInLong -= 2;
		}

		pixels -= 2;
	}

	if (pixels) {		// pixelsInLong is at least 1
		uint8_t pixel = eightPixels >> (((8 - pixelsInLong) ^ 1) << 2);
		*to = (pixel << 4) | (*to & 0x0F);
	}
}

void copy4MisalignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels)
{
	uint8_t offset = 3 & (uintptr_t)from;
	uint32_t eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);
	uint8_t pixelsInLong = 6 - (offset << 1);

	from += (4 - offset);

	eightPixels >>= (offset << 3);

	*to = (*to & 0xF0) | (eightPixels & 0x0F);
	to++;

	eightPixels >>= 8;
	pixels -= 1;

	while (pixels >= 2) {
		if (0 == pixelsInLong) {
			eightPixels = *(uint32_t *)from;
			from += 4;
			pixelsInLong = 6;

			*to++ = eightPixels;
			eightPixels >>= 8;
		}
		else {
			*to++ = eightPixels;
			eightPixels >>= 8;
			pixelsInLong -= 2;
		}

		pixels -= 2;
	}

	if (pixels) {
		if (0 ==  pixelsInLong)
			eightPixels = *(uint32_t *)from;
		*to = (eightPixels & 0xF0) | (*to & 0x0F);
	}
}
#else

void copy4AlignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap)
{
	uint16_t bytes = pixels >> 1;

	while (bytes--) {
		uint8_t twoPixels = c_read8(from++);		// @@
		*to++ = PocoPixels4Pack(remap[twoPixels >> 4], remap[twoPixels & 0x0f]);
	}

	if (1 & pixels) {
		uint8_t offset;
		uint32_t eightPixels;
		uint8_t pixel;

		offset = 3 & (uintptr_t)from;
		eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);

		pixel = eightPixels >> (((offset << 1) ^ 1) << 2);		// eightPixels pixels ordered: 6 7  4 5  2 3  0 1
		*to = (remap[pixel] << kPocoPixels4FirstShift) | (*to & kPocoPixels4SecondMask);
	}
}

void copy4MisalignedSourceAlignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap)
{
	uint8_t offset = 3 & (uintptr_t)from;
	uint32_t eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);
	uint8_t pixelsInLong = (8 - (offset << 1)) - 1;

	from += (4 - offset);

	while (pixels >= 2) {
		if (1 == pixelsInLong) {
			uint8_t twoPixels = remap[(eightPixels >> 24) & 0x0F];

			eightPixels = *(uint32_t *)from;
			from += 4;
			pixelsInLong = 7;

			*to++ = PocoPixels4Pack(twoPixels, remap[((eightPixels >> 4) & 0x0F)]);
		}
		else {
			uint16_t fourPixels = eightPixels >> ((7 - pixelsInLong) << 2);			// 7, 5, or 3 pixelsInLong
			*to++ = PocoPixels4Pack(remap[fourPixels & 0x0F], remap[(uint8_t)(fourPixels >> 12)]);
			pixelsInLong -= 2;
		}

		pixels -= 2;
	}

	if (pixels) {		// pixelsInLong is at least 1
		uint8_t pixel = eightPixels >> (((8 - pixelsInLong) ^ 1) << 2);
		*to = (remap[pixel & 0x0F] << kPocoPixels4FirstShift) | (*to & kPocoPixels4SecondMask);
	}
}

void copy4AlignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap)
{
	uint8_t offset = 3 & (uintptr_t)from;
	uint32_t eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);
	uint8_t pixelsInLong = (8 - (offset << 1));
	uint8_t pixel;

	from += (4 - offset);

	pixel = (eightPixels >> (((8 - pixelsInLong) ^ 1) << 2)) & 0x0F;
	*to = (*to & kPocoPixels4FirstMask) | (remap[pixel] << kPocoPixels4SecondShift);
	to++;
	pixels -= 1;
	pixelsInLong -= 1;

	while (pixels >= 2) {
		if (1 == pixelsInLong) {
			uint8_t twoPixels = remap[(eightPixels >> 24) & 0x0F];

			eightPixels = *(uint32_t *)from;
			from += 4;
			pixelsInLong = 7;

			*to++ = PocoPixels4Pack(twoPixels, remap[((eightPixels >> 4) & 0x0F)]);
		}
		else {
			uint16_t fourPixels = eightPixels >> ((7 - pixelsInLong) << 2);			// 7, 5, or 3 pixelsInLong
			*to++ = PocoPixels4Pack(remap[fourPixels & 0x0F], remap[(uint8_t)(fourPixels >> 12)]);
			pixelsInLong -= 2;
		}

		pixels -= 2;
	}

	if (pixels) {		// pixelsInLong is at least 1
		uint8_t pixel = eightPixels >> (((8 - pixelsInLong) ^ 1) << 2);
		*to = (remap[pixel & 0x0F] << kPocoPixels4FirstShift) | (*to & kPocoPixels4SecondMask);
	}
}

void copy4MisalignedSourceMisalignedDest(uint8_t *to, const uint8_t *from, uint16_t pixels, uint8_t *remap)
{
	uint8_t offset = 3 & (uintptr_t)from;
	uint32_t eightPixels = *(uint32_t *)(-offset + (uintptr_t)from);
	uint8_t pixelsInLong = 6 - (offset << 1);

	from += (4 - offset);

	eightPixels >>= (offset << 3);

	*to = (*to & kPocoPixels4FirstMask) | (remap[eightPixels & 0x0F] << kPocoPixels4SecondShift);
	to++;

	eightPixels >>= 8;
	pixels -= 1;

	while (pixels >= 2) {
		if (0 == pixelsInLong) {
			eightPixels = *(uint32_t *)from;
			from += 4;
			pixelsInLong = 6;

			*to++ = PocoPixels4Pack(remap[((uint8_t)eightPixels) >> 4], remap[eightPixels & 0x0F]);
			eightPixels >>= 8;
		}
		else {
			*to++ = PocoPixels4Pack(remap[((uint8_t)eightPixels) >> 4], remap[eightPixels & 0x0F]);
			eightPixels >>= 8;
			pixelsInLong -= 2;
		}

		pixels -= 2;
	}

	if (pixels) {
		if (0 ==  pixelsInLong)
			eightPixels = *(uint32_t *)from;
		*to = (remap[(eightPixels & 0xF0) >> 4] << kPocoPixels4FirstShift) | (*to & kPocoPixels4SecondMask);
	}
}

#endif
#endif

#if kPocoPixelFormat == kCommodettoBitmapCLUT16

void buildColorMap(uint32_t *srcCLUT, uint8_t *inverseTable, uint8_t *remap)
{
	uint8_t i;
	for (i = 0; i < 16; i++, srcCLUT++) {
		uint32_t entry = c_read32(srcCLUT);
		uint16_t srcColor;
		uint16_t c;

		c = entry & 0xFF;
		srcColor = (c >= 248) ? 0x0F : (c + 7) >> 4;

		c = (entry >> 8) & 0xFF;
		srcColor |= (c >= 248) ? 0xF0 : (c + 7) & 0xF0;

		c = (entry >> 16) & 0xFF;
		srcColor |= (c >= 248) ? 0xF00 : ((c + 7) & 0xF0) << 4;

		remap[i] = c_read8(inverseTable + srcColor);
	}
}
#endif

#define kReuse0Mask (0x10)
#define kReuse1Mask (0x08)

void doDrawFrame(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	Frame f = (Frame)pc;
	uint8_t yOffset = f->yOffset;
	PocoPixel prev0 = f->prev0, prev1 = f->prev1;
	PocoCoordinate rowBump = (poco->rowBytes >> (sizeof(PocoPixel) - 1)) - f->w;

	while (h--) {
		const uint8_t *data;
		uint16_t widthRemain = f->w;
		PocoPixel color0, color1;
		uint8_t clipLeft = f->clipLeft & 3;
		uint8_t repeatSkip;

		if (f->clipLeft >= 4) {
			const uint8_t *td = f->data;
			PocoPixel tp0 = f->prev0, tp1 = f->prev1;
			repeatSkip = doSkipColorCells(poco, pc, f->clipLeft >> 2);
			data = f->data;
			prev0 = f->prev0;
			prev1 = f->prev1;
			f->data = td;
			f->prev0 = tp0;
			f->prev1 = tp1;
		}
		else {
			repeatSkip = 0;
			data = f->data;
		}

		do {
			uint8_t command = c_read8(data++);
			uint32_t bitmask;
			uint16_t out[4];
			uint8_t repeat = 1;

			switch (command >> 5) {
				case 0:		// 1 color
					if (command & kReuse0Mask)
						color0 = prev0;
					else if (command & kReuse1Mask)
						color0 = prev1;
					else {
						color0 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
						prev0 = color0;
					}
					repeat = (command & 7) + 1 - repeatSkip;
					repeatSkip = 0;
#if 1
					out[0] = out[1] = out[2] = out[3] = color0;
#else
					out[0] = out[1] = out[2] = out[3] = 0;
#endif
					break;

				case 4:
					command &= 0x1f;
					prev0 = color0 = (command << 11) | (command << 6) | command;		//@@ low bit of green
#if 1
					out[0] = out[1] = out[2] = out[3] = color0;
#else
					out[0] = out[1] = out[2] = out[3] = 0;
#endif
					break;

				case 5:
					command &= 0x1f;
					prev0 = color0 = (command << 11) | (command << 6) | command;		//@@ low bit of green
					command = c_read8(data++);
					prev1 = color1 = (command << 11) | (command << 6) | command;		//@@ low bit of green
					goto twoColorCommon;

				case 1:		// 2 color
					if (!(command & kReuse0Mask)) {
						prev0 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
					}
					color0 = prev0;

					if (!(command & kReuse1Mask)) {
						prev1 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
					}
					color1 = prev1;
twoColorCommon:
					bitmask = c_read16(data);
					data += 2;
					bitmask >>= (3 - yOffset) << 2;
#if 1
					out[0] = (bitmask & 0x08) ? color1 : color0;
					out[1] = (bitmask & 0x04) ? color1 : color0;
					out[2] = (bitmask & 0x02) ? color1 : color0;
					out[3] = (bitmask & 0x01) ? color1 : color0;
#else
					out[0] = out[1] = out[2] = out[3] = 0;
#endif
					break;

				case 2:		// 4 color
					if (!(command & kReuse0Mask)) {
						prev0 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
					}
					color0 = prev0;

					if (!(command & kReuse1Mask)) {
						prev1 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
					}
					color1 = prev1;

					bitmask = c_read32(data);
					data += 4;
					bitmask >>= (3 - yOffset) << 3;
#if 1
					{
					uint8_t i;
					uint8_t r0 = color0 >> 11;
					uint8_t g0 = (color0 >> 5) & 0x3f;
					uint8_t b0 = color0 & 0x1f;
					uint8_t r1 = color1 >> 11;
					uint8_t g1 = (color1 >> 5) & 0x3f;
					uint8_t b1 = color1 & 0x1f;
					uint8_t ir0 = ((r0 + r1 + r1 + r1) + 2) >> 2;
					uint8_t ir1 = ((r0 + r0 + r0 + r1) + 2) >> 2;
					uint8_t ig0 = ((g0 + g1 + g1 + g1) + 2) >> 2;
					uint8_t ig1 = ((g0 + g0 + g0 + g1) + 2) >> 2;
					uint8_t ib0 = ((b0 + b1 + b1 + b1) + 2) >> 2;
					uint8_t ib1 = ((b0 + b0 + b0 + b1) + 2) >> 2;
					PocoPixel color0i = (ir0 << 11) | (ig0 << 5) | ib0;
					PocoPixel color1i = (ir1 << 11) | (ig1 << 5) | ib1;

					for (i = 0; i < 4; i++) {
						switch ((bitmask >> 6) & 3) {
							case 0:	out[i] = color0; break;
							case 1:	out[i] = color0i; break;
							case 2:	out[i] = color1i; break;
							case 3:	out[i] = color1; break;
						}
						bitmask <<= 2;
					}
					}
#else
					out[0] = out[1] = out[2] = out[3] = 0;
#endif
					break;

				case 3:		// 16 color
					{
					const uint8_t *dp = data + yOffset * 4 * sizeof(PocoPixel);

#if 1
					out[0] = c_read16(dp);
					out[1] = c_read16(dp + 2);
					out[2] = c_read16(dp + 4);
					out[3] = c_read16(dp + 6);
					dp += 8;
#else
					out[0] = out[1] = out[2] = out[3] = 0;
#endif
					data += 16 * sizeof(PocoPixel);
					}
					break;

				default:
					return;		// bad data
			}

			do {
				PocoPixel *src = (PocoPixel *)out;
				uint8_t count = widthRemain & 3;
				if (!count) count = 4;

				if (clipLeft) {
					if (clipLeft >= count)
						count = 0;
					else
						count -= clipLeft;
					src += clipLeft;
					clipLeft = 0;
				}

				dst[0] = src[0];
				if (4 == count) {
					dst[1] = src[1];
					dst[2] = src[2];
					dst[3] = src[3];
				}
				else if (3 == count) {
					dst[1] = src[1];
					dst[2] = src[2];
				}
				else if (2 == count)
					dst[1] = src[1];

				dst += count;
				widthRemain -= count;
			} while (--repeat && widthRemain);
		} while (widthRemain);

		yOffset += 1;
		if (4 == yOffset) {
			if (f->blockWidth != f->unclippedBlockWidth) {
				doSkipColorCells(poco, pc, f->blockWidth);
				data = f->data;
				prev0 = f->prev0;
				prev1 = f->prev1;
			}
			else {
				f->data = data;
				f->prev0 = prev0;
				f->prev1 = prev1;
			}

			yOffset = 0;
		}
		else {
			prev0 = f->prev0;
			prev1 = f->prev1;
		}

		dst += rowBump;
	}

	f->yOffset = yOffset;
}

uint8_t doSkipColorCells(Poco poco, PocoCommand pc, int cells)
{
	Frame f = (Frame)pc;
	PocoPixel prev0 = f->prev0, prev1 = f->prev1;
	const uint8_t *data = f->data;
	PocoPixel color0;
	uint8_t repeat;
	uint8_t repeatSkip = 0;

	while (cells--) {
		uint8_t command = c_read8(data++);

		switch (command >> 5) {
			case 0:		// 1 color
				repeat = (command & 7) + 1;
				if ((cells + 1) < repeat) {
					repeatSkip = (cells + 1);
					data -= 1;
					goto done;		// this keeps ->data pointing to this cell
				}

				if (command & kReuse0Mask)
					color0 = prev0;
				else if (command & kReuse1Mask)
					color0 = prev1;
				else {
					color0 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
					prev0 = color0;
				}
				cells -= repeat - 1;		// cells is decremented on the loop
				break;

			case 1:		// 2 color
				if (!(command & kReuse0Mask)) {
					prev0 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
				}
				color0 = prev0;

				if (!(command & kReuse1Mask)) {
					prev1 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
				}

				data += 2;
				break;

			case 2:		// 4 color
				if (!(command & kReuse0Mask)) {
					prev0 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
				}
				color0 = prev0;

				if (!(command & kReuse1Mask)) {
					prev1 = c_read16(data); data += 2;		//@@ assumes 16 bit pixel
				}

				data += 4;
				break;

			case 3:		// 16 color
				data += 16 * sizeof(PocoPixel);
				break;

			case 4:		// 1 color gray
				command &= 0x1f;
				color0 = prev0 = (command << 11) | (command << 6) | command;		//@@ low bit of green
				break;

			case 5:		// 2 color gray
				command &= 0x1f;
				color0 = prev0 = (command << 11) | (command << 6) | command;		//@@ low bit of green
				command = c_read8(data); data += 3;
				break;

			default:
				return 0;		// bad data
		}
	}

done:
	f->data = data;
	f->prev0 = prev0;
	f->prev1 = prev1;

	return repeatSkip;
}

void doDrawExternal(Poco poco, PocoCommand pc, PocoPixel *dst, PocoDimension h)
{
	External e = (External)pc;

#if kPocoPixelSize < 8
	(e->doDrawExternal)(poco, e->data, dst, pc->w, h, pc->xphase);
#else
	(e->doDrawExternal)(poco, e->data, dst, pc->w, h, 0);
#endif
}

void PocoDrawingBegin(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	// clip against the display
	if (x < 0) {
		w += x;
		x = 0;
	}
	else if (x >= poco->width)
		w = 0;

	if (y < 0) {
		h += y;
		y = 0;
	}
	else if (y >= poco->height)
		h = 0;

	if ((x + w) > poco->width)
		w = poco->width - x;
	if ((y + h) > poco->height)
		h = poco->height - y;

	// rotate
	rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);

	// get started
	poco->next = (PocoCommand)poco->displayList;
	poco->flags &= ~(kPocoFlagErrorDisplayListOverflow | kPocoFlagErrorStackProblem | kPocoFlagGCDisabled);
	poco->stackDepth = 0;
	poco->xOrigin = poco->yOrigin = 0;

	poco->x = x;
	poco->y = y;
	poco->w = w;
	poco->h = h;

	poco->xMax = x + w;
	poco->yMax = y + h;

	poco->flags |= kPocoFlagDidBegin;

#if kPocoFrameBuffer
	poco->frameBuffer = NULL;
#endif
 }

int PocoDrawingEnd(Poco poco, PocoPixel *pixels, int byteLength, PocoRenderedPixelsReceiver pixelReceiver, void *refCon)
{
	PocoCoordinate yMin, yMax;
	int16_t rowBytes;
	int16_t displayLines;
	PocoCommand displayList, displayListEnd;
	PocoCommand walker;
	PocoPixel *pixelsAlt;

	if (!(poco->flags & kPocoFlagDidBegin))
		return 4;
	poco->flags &= ~kPocoFlagDidBegin;

	if (poco->stackDepth) {
		poco->displayListEnd += poco->stackDepth * sizeof(PocoRectangleRecord); 
		return 2;
	}

	if (poco->flags & kPocoFlagErrorDisplayListOverflow)
		return 1;

	if (poco->flags & kPocoFlagErrorStackProblem)
		return 3;

	if (poco->displayList == (char *)poco->next)		// empty. Could be no drawing or could be 0 width and/or height
		return 0;

	pocoInstrumentationMax(PocoDisplayListUsed, (char *)poco->next - (char *)poco->displayList);

	rowBytes = ((poco->w * kPocoPixelSize) + 7) >> 3;
	poco->rowBytes = rowBytes;
 
	int canAsync = ((byteLength >> 1) >= rowBytes) && (kPocoFlagDoubleBuffer & poco->flags);
	if (canAsync)
		displayLines = (byteLength >> 1) / rowBytes;
	else
		displayLines = byteLength / rowBytes;
	if (displayLines <= 0) return 4;

	if (canAsync) {
		pixelsAlt = (PocoPixel *)((byteLength >> 1) + (char *)pixels);
		if (3 & (uintptr_t)pixelsAlt)
			pixelsAlt = (PocoPixel *)((4 + (uintptr_t)pixelsAlt) & ~3);
		
		if (kPocoFlagBuffer & poco->flags) {
			PocoPixel *tp = pixels;
			pixels = pixelsAlt;
			pixelsAlt = tp;
		}
	}
	else {
		pixelsAlt = NULL;
	}

	displayList = (PocoCommand)poco->displayList;
	displayListEnd = poco->next;

	for (walker = displayList; walker != displayListEnd; walker = (PocoCommand)(walker->length + (char *)walker)) {
		walker->x -= poco->x;
#if 4 == kPocoPixelSize
		walker->xphase = walker->x & 1;
		walker->x >>= 1;
#endif
	}

	// walk through a slab of displayList at a time
	for (yMin = poco->y; yMin < poco->yMax; yMin = yMax) {
		yMax = yMin + displayLines;
		if (yMax > poco->yMax)
			yMax = poco->yMax;

		for (walker = displayList; walker != displayListEnd; walker = (PocoCommand)(walker->length + (char *)walker)) {
			PocoCoordinate y = walker->y;
			PocoDimension h;

			if (y >= yMax)
				continue;						// completely below the slab being drawn

			h = walker->h;

			if (y < yMin) {
				if ((y + h) <= yMin) {				// full object above the slab being drawn
					walker->y = poco->yMax;			// move it below bottom, so this loop can exit early next time
					continue;
				}

				h -= (yMin - y);
				y = yMin;
			}
			if ((y + h) > yMax)
				h = yMax - y;

#if 4 != kPocoPixelSize
			(gDrawRenderCommand[walker->command])(poco, walker, pixels + ((y - yMin) * poco->w) + walker->x, h);
#else
			(gDrawRenderCommand[walker->command])(poco, walker, pixels + ((y - yMin) * rowBytes) + walker->x, h);
#endif
		}

		if (pixelsAlt) {
			(pixelReceiver)(pixels, -(rowBytes * (yMax - yMin)), refCon);		// negative length means async

			PocoPixel *tp = pixels;
			pixels = pixelsAlt;
			pixelsAlt = tp;

			poco->flags ^= kPocoFlagBuffer;
		}
		else {
			(pixelReceiver)(pixels, rowBytes * (yMax - yMin), refCon);
		}
	}

	pocoInstrumentationAdjust(PixelsDrawn, poco->w * (poco->yMax - poco->y));

	return 0;
}

#if kPocoFrameBuffer
void PocoDrawingBeginFrameBuffer(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h, PocoPixel *frameBuffer, int16_t rowBytes)
{
	PocoDrawingBegin(poco, 0, 0, poco->width, poco->height);
	PocoClipPush(poco, x, y, w, h);
	poco->frameBuffer = frameBuffer;
	poco->rowBytes = rowBytes;
}

int PocoDrawingEndFrameBuffer(Poco poco)
{
	PocoClipPop(poco);
	poco->frameBuffer = NULL;

	return 0;
}
#endif

int PocoClipPush(Poco poco, PocoCoordinate x, PocoCoordinate y, PocoDimension w, PocoDimension h)
{
	PocoCoordinate xMax, yMax;
	PocoRectangle clip = (PocoRectangle)(poco->displayListEnd - sizeof(PocoRectangleRecord)); 
	if ((char *)clip < (char *)poco->next) {
		poco->flags |= kPocoFlagErrorStackProblem;
		pocoInstrumentationMax(PocoDisplayListUsed, (char *)poco->next - (char *)poco->displayList);
		return 0;
	}

	rotateCoordinatesAndDimensions(poco->width, poco->height, x, y, w, h);

	poco->stackDepth++;
	poco->displayListEnd = (char *)clip;

	// save current clip
	clip->x = poco->x;
	clip->y = poco->y;
	clip->w = poco->w;
	clip->h = poco->h;

	// intersect new clip with current clip
	xMax = x + w;
	yMax = y + h;

	if (x < poco->x)
		x = poco->x;

	if (xMax > poco->xMax)
		xMax = poco->xMax;

	if (y < poco->y)
		y = poco->y;

	if (yMax > poco->yMax)
		yMax = poco->yMax;

	if ((x >= xMax) || (y >= yMax)) {
		// clipped out
		poco->x = 0;
		poco->y = 0;
		poco->w = 0;
		poco->h = 0;
		poco->xMax = 0;
		poco->yMax = 0;
		return 0;
	}

	// apply new clip
	poco->x = x;
	poco->y = y;
	poco->w = xMax - x;
	poco->h = yMax - y;
	poco->xMax = xMax;
	poco->yMax = yMax;

	return 1;
}

void PocoClipPop(Poco poco)
{
	PocoRectangle clip = (PocoRectangle)poco->displayListEnd;

	if (0 == poco->stackDepth) {
		poco->flags |= kPocoFlagErrorStackProblem;
		return;
	}

	poco->x = clip->x;
	poco->y = clip->y;
	poco->w = clip->w;
	poco->h = clip->h;
	poco->xMax = clip->x + clip->w;
	poco->yMax = clip->y + clip->h;
	
	poco->displayListEnd = (char *)(clip + 1);
	poco->stackDepth--;
}

void PocoOriginPush(Poco poco, PocoCoordinate x, PocoCoordinate y)
{
	PocoRectangle clip = (PocoRectangle)(poco->displayListEnd - sizeof(PocoRectangleRecord)); 
	if ((char *)clip < (char *)poco->next) {
		poco->flags |= kPocoFlagErrorStackProblem;
		pocoInstrumentationMax(PocoDisplayListUsed, (char *)poco->next - (char *)poco->displayList);
		return;
	}

	poco->stackDepth++;
	poco->displayListEnd = (char *)clip;

	// save current origin
	clip->x = poco->xOrigin;
	clip->y = poco->yOrigin;

	// apply new origin
	poco->xOrigin += x;
	poco->yOrigin += y;
}

void PocoOriginPop(Poco poco)
{
	PocoRectangle clip = (PocoRectangle)poco->displayListEnd;

	if (0 == poco->stackDepth) {
		poco->flags |= kPocoFlagErrorStackProblem;
		return;
	}

	poco->xOrigin = clip->x;
	poco->yOrigin = clip->y;

	poco->displayListEnd = (char *)(clip + 1);
	poco->stackDepth--;
}



