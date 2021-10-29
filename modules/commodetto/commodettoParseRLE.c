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
#include "commodettoBitmap.h"

#include "xsmc.h"

void xs_parseRLE(xsMachine *the)
{
	unsigned char *bytes;
	CommodettoBitmap bitmap;
	xsUnsignedValue dataSize;

	if (xsBufferRelocatable == xsmcGetBufferReadable(xsArg(0), (void **)&bytes, &dataSize))
		xsUnknownError("invaild");

	if ((109 != c_read8(bytes + 0)) || (100 != c_read8(bytes + 1)))		// "md"
		xsUnknownError("invalid commodetto rle");

	if (0 != c_read8(bytes + 2))
		xsUnknownError("unsupported version");

//	if ((kCommodettoBitmapGray16 | kCommodettoBitmapPacked) != c_read8(bytes + 3))
//		xsUnknownError("unsupported pixel format");

	bitmap = xsmcGetHostChunk(xsArg(1));
	bitmap->w = (c_read8(bytes + 4) << 8) | c_read8(bytes + 5);
	bitmap->h = (c_read8(bytes + 6) << 8) | c_read8(bytes + 7);
	bitmap->format = c_read8(bytes + 3);
	bitmap->havePointer = 1;
	bitmap->bits.data = bytes + 8;		//@@ only valid if from Resource

	xsResult = xsArg(1);
}
