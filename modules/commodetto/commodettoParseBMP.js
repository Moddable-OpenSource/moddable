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


/*
	Commodetto BMP parser

		trivial validation
		identifies gray palettes
		extracts width, height and offset to pixels

*/

import Bitmap from "commodetto/Bitmap";

function parse() @ "xs_parseBMP";

export default function (buffer) {
	let bitmap = new Bitmap(1, 1, Bitmap.Default, buffer, 0);
	parse(buffer, bitmap);
	return bitmap;
}

/*
export default function parseBMP(buffer) {
	let bytes = new Uint8Array(buffer);

	if ((66 != bytes[0]) || (77 != bytes[1]))		// "BM"
		throw new Error("invalid BMP");

	let offset = bytes[10] | (bytes[11] << 8) | (bytes[12] << 16)| (bytes[13] << 24);
	let size = bytes[14] | (bytes[15] << 8);		// biSize
	let width = bytes[18] | (bytes[19] << 8) | (bytes[20] << 16)| (bytes[21] << 24);
	let height = bytes[22] | (bytes[23] << 8) | (bytes[24] << 16)| (bytes[25] << 24);
	if (bytes[25] & 0x80) {	// negative value... ouch. means BMP is upside down.
		height = ~height;
		height += 1;
	}

	let depth = bytes[28] | (bytes[29] << 8);
	let compression = bytes[30] | (bytes[31] << 8) | (bytes[32] << 16)| (bytes[33] << 24);			// biCompression

	if (8 == depth) {
		if (0 != compression)
			throw new Error("unsupported 8-bit compression");

		if (width & 3)
			throw new Error("8-bit bitmap width must be multiple of 4");

		for (let palette = size + 14, gray = 0; gray < 256; gray++, palette += 4) {
			if ((gray != bytes[palette + 0]) || (gray != bytes[palette + 1]) || (gray != bytes[palette + 2]))
				return new Bitmap(width, height, Bitmap.RGB332, buffer, offset);		//@@ CHECK PALETTE
		}

		return new Bitmap(width, height, Bitmap.Gray256, buffer, offset);
	}

	if (4 == depth) {
		if (0 != compression)
			throw new Error("unsupported 4-bit compression");

		if (width & 7)
			throw new Error("4-bit bitmap width must be multiple of 8");

		let format = Bitmap.Gray16;
		let palette = size + 14;
		for (let i = 0; i < 16; i++, palette += 4) {
			let gray = i | (i << 4);
			if ((gray != bytes[palette + 0]) || (gray != bytes[palette + 1]) || (gray != bytes[palette + 2])) {
				format = Bitmap.CLUT16;
				palette += (16 - i) * 4;
				if (offset != palette)
					throw new Error("pixels must immediately follow palette");
				break;
			}
		}

		return new Bitmap(width, height, format, buffer, offset);
	}

	if (1 == depth) {
		if (0 != compression)
			throw new Error("unsupported 1-bit compression");

		if (width & 31)
			throw new Error("1-bit BMP width must be multiple of 32");

		return new Bitmap(width, height, Bitmap.Monochrome, buffer, offset);
	}

	if (16 == depth) {
		if (3 !== compression)
			throw new Error("must be 565 pixels");

		if (width & 1)
			throw new Error("width not multiple of 2");

		return new Bitmap(width, height, Bitmap.RGB565LE, buffer, offset);
	}

	throw new Error("unsupported BMP depth - must be 16, 8, 4, or 1");
}
*/
