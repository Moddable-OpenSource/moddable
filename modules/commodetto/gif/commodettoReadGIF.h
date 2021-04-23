/*
* Copyright (c) 2021  Moddable Tech, Inc.
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

#ifndef __commodetto_ReadGIF_h__
#define __commodetto_ReadGIF_h__

#include "commodettoBitmap.h"
#include "AnimatedGIF.h"

typedef struct {
	CommodettoBitmapRecord bitmap;
	int			frameNumber;
	uint16_t	frameX;
	uint16_t	frameY;
	uint16_t	frameW;
	uint16_t	frameH;
	uint16_t	background;
	uint16_t	transparentColor;
	uint8_t		hasTransparentColor;
	uint8_t		hasTransparentBackground;
	uint8_t		ready;		// 0 - not ready, 1 - at least one frame, 2 - all frames
	uint8_t		reduced;
	int32_t		bufferSize;
	// everything after here is freed immediately for single image GIFs
	uint8_t		disposalMethod;
	uint16_t	prevX;
	uint16_t	prevY;
	uint16_t	prevW;
	uint16_t	prevH;
	void		*pixels;
	GIFIMAGE	gi;
	GIFINFO		ginfo;
} xsGIFRecord, *xsGIF;

#endif
