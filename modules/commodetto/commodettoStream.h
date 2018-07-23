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


#ifndef __commodetto_Stream_h__
#define __commodetto_Stream_h__

#include <stdint.h>

// N.B. packed - represents bitstream

typedef struct ColorCellHeaderRecord {
	uint8_t		id_c;
	uint8_t		id_s;
	uint8_t		bitmapFormat;
	uint8_t		reserved;
	uint16_t	width;
	uint16_t	height;
	uint16_t	frameCount;
	uint16_t	fps_numerator;
	uint16_t	fps_denominator;
//	uint16_t	bytes;		// each frame is a 16 bit byte length followed by the frame data
} ColorCellHeaderRecord, *ColorCellHeader;

#endif
