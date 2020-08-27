/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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


#ifndef __commodetto_Bitmap_h__
#define __commodetto_Bitmap_h__

#include <stdint.h>

#include "commodettoBitmapFormat.h"
#if !XSTOOLS
	#include "mc.defines.h"
	#ifdef MODDEF_COMMODETTO_BITMAP_ID
		#define COMMODETTO_BITMAP_ID MODDEF_COMMODETTO_BITMAP_ID
	#endif
#endif

#ifndef kCommodettoBitmapFormat
	#define kCommodettoBitmapFormat kCommodettoBitmapRGB565LE
#endif

#if (kCommodettoBitmapFormat == kCommodettoBitmapGray16) || (kCommodettoBitmapFormat == kCommodettoBitmapCLUT16)
	#define kCommodettoPixelSize (4)
#elif (kCommodettoBitmapFormat == kCommodettoBitmapGray256) || (kCommodettoBitmapFormat == kCommodettoBitmapRGB332)
	#define kCommodettoPixelSize (8)
#elif (kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE) || (kCommodettoBitmapFormat == kCommodettoBitmapRGB565BE)
	#define kCommodettoPixelSize (16)
#else
	#error
#endif

typedef uint8_t CommodettoBitmapFormat;
typedef int16_t CommodettoCoordinate;
typedef uint16_t CommodettoDimension;

typedef struct {
	CommodettoCoordinate	x;
	CommodettoCoordinate	y;
	CommodettoDimension		w;
	CommodettoDimension		h;
} CommodettoRectangleRecord, *CommodettoRectangle;

#if 4 == kCommodettoPixelSize
	typedef uint8_t CommodettoPixel;
#elif 8 == kCommodettoPixelSize
	typedef uint8_t CommodettoPixel;
#elif 16 == kCommodettoPixelSize
	typedef uint16_t CommodettoPixel;
#elif 32 == kCommodettoPixelSize
	typedef uint32_t CommodettoPixel;
#else
	#error
#endif

#define kCommodettoBitmapHaveByteLength (1)

typedef struct {
	CommodettoDimension		w;
	CommodettoDimension		h;
	CommodettoBitmapFormat	format;
	int8_t					havePointer;
	int8_t					flags;
	union {
		void				*data;
		int32_t				offset;
	} bits;
#if COMMODETTO_BITMAP_ID
	uint32_t				id;
#endif
	int32_t					byteLength;			// must be last
} CommodettoBitmapRecord, *CommodettoBitmap;

uint8_t CommodettoBitmapGetDepth(CommodettoBitmapFormat format);

#endif
