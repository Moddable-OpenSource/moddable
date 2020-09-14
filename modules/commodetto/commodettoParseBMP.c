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
#include "mc.xs.h"			// for xsID_ values

void xs_parseBMP(xsMachine *the)
{
	unsigned char *bytes;
	uint32_t palette;
	uint16_t gray, i;
	CommodettoBitmap bitmap;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
		bytes = xsmcToArrayBuffer(xsArg(0));
	else
		bytes = xsmcGetHostData(xsArg(0));

	if ((66 != c_read8(bytes + 0)) || (77 != c_read8(bytes + 1)))		// "BM"
		xsUnknownError("invalid BMP");

	uint32_t offset = c_read8(bytes + 10) | (c_read8(bytes + 11) << 8) | (c_read8(bytes + 12) << 16) | (c_read8(bytes + 13) << 24);
	uint16_t size = c_read8(bytes + 14) | (c_read8(bytes + 15) << 8);		// biSize
	uint32_t width = c_read8(bytes + 18) | (c_read8(bytes + 19) << 8) | (c_read8(bytes + 20) << 16) | (c_read8(bytes + 21) << 24);
	uint32_t height = c_read8(bytes + 22) | (c_read8(bytes + 23) << 8) | (c_read8(bytes + 24) << 16) | ((uint32_t)c_read8(bytes + 25) << 24);
	if (c_read8(bytes + 25) & 0x80) {	// negative value... ouch. means BMP is upside down.
		height = ~height;
		height += 1;
	}

	uint16_t depth = c_read8(bytes + 28) | (c_read8(bytes + 29) << 8);
	uint32_t compression = c_read8(bytes + 30) | (c_read8(bytes + 31) << 8) | (c_read8(bytes + 32) << 16)| (c_read8(bytes + 33) << 24);			// biCompression

	bitmap = xsmcGetHostChunk(xsArg(1));
	bitmap->w = width;
	bitmap->h = height;
	if (bitmap->havePointer)
		bitmap->bits.data = bytes + offset;
	else
		bitmap->bits.offset = offset;

	switch (depth) {
		case 8:
			if (0 != compression)
				xsUnknownError("unsupported 8-bit compression");

			if (width & 3)
				xsUnknownError("8-bit bitmap width must be multiple of 4");

			for (palette = size + 14, gray = 0; gray < 256; gray++, palette += 4) {
				if ((gray != c_read8(bytes + palette + 0)) || (gray != c_read8(bytes + palette + 1)) || (gray != c_read8(bytes + palette + 2)))
					bitmap->format = kCommodettoBitmapRGB332;		//@@ CHECK PALETTE
					return;
			}

			bitmap->format = kCommodettoBitmapGray256;
			break;

		case 4:
			if (0 != compression)
				xsUnknownError("unsupported 4-bit compression");

			if (width & 7)
				xsUnknownError("4-bit bitmap width must be multiple of 8");

			for (palette = size + 14, i = 0; i < 16; i++, palette += 4) {
				gray = i | (i << 4);
				if ((gray != c_read8(bytes + palette + 0)) || (gray != c_read8(bytes+ palette + 1)) || (gray != c_read8((bytes + palette + 2)))) {
					palette += (16 - i) * 4;
					if (offset != palette)
						xsUnknownError("pixels must immediately follow palette");

					bitmap->format = kCommodettoBitmapCLUT16;
					return;
				}
			}
			bitmap->format = kCommodettoBitmapGray16;

			break;

		case 1:
			if (0 != compression)
				xsUnknownError("unsupported 1-bit compression");

			if (width & 31)
				xsUnknownError("1-bit BMP width must be multiple of 32");

			bitmap->format = kCommodettoBitmapMonochrome;
			break;

		case 16:
			if (3 !=compression)
				xsUnknownError("must be 565 pixels");

			if (width & 1)
				xsUnknownError("width not multiple of 2");

			if ((0x0f00 == c_read32(54 + bytes)) &&
				(0x00f0 == c_read32(58 + bytes)) &&
				(0x000f == c_read32(62 + bytes)) &&
				(0xf000 == c_read32(66 + bytes)))
				bitmap->format = kCommodettoBitmapARGB4444;
			else
				bitmap->format = kCommodettoBitmapRGB565LE;
			break;

		default:
			xsUnknownError("unsupported BMP depth - must be 16, 8, 4, or 1");
			break;

	}
}
