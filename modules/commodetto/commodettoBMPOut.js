/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
	Commodetto BMP Output
		
		implements PixelOut to write pixels to a BMP file

	//@@ rework so it accepts file object
*/

import Bitmap from "commodetto/Bitmap";
import PixelsOut from "commodetto/PixelsOut";
import File from "file";

export default class BMPOut extends PixelsOut {
	constructor(dictionary) {
		super(dictionary);

		this.depth = Bitmap.depth(dictionary.pixelFormat);

		if ((Bitmap.Gray16 != dictionary.pixelFormat) && (Bitmap.Gray256 != dictionary.pixelFormat) && (Bitmap.RGB565LE != dictionary.pixelFormat) && (Bitmap.RGB332 != dictionary.pixelFormat) && (Bitmap.CLUT16 != dictionary.pixelFormat) && (Bitmap.Monochrome != dictionary.pixelFormat) &&
			(Bitmap.ARGB4444 != dictionary.pixelFormat) && (Bitmap.BGRA32 != dictionary.pixelFormat))
			throw new Error("unsupported BMP pixel fornat");

		this.file = new File(dictionary.path, 1);

		if (dictionary.clut)
			this.colors = dictionary.clut;
	}
	begin(x, y, width, height) {
		let rowBytes = this.pixelsToBytes(width);

		this.file.length = 0;
		this.file.position = 0;

		if (32 == this.depth) {
			this.file.write("BM");						// imageFileType
			this.write32((rowBytes * height) + 0x38);	// fileSize
			this.write16(0);							// reserved1
			this.write16(0);							// reserved2
			this.write32(0x36);							// imageDataOffset

			this.write32(0x28);							// biSize
			this.write32(width);						// biWidth
			this.write32(-height);						// biHeight (negative, because we write top-to-bottom)

			this.write16(1);							// biPlanes
			this.write16(32);							// biBitCount
			this.write32(0);							// biCompression
			this.write32((rowBytes * height) + 2);		// biSizeImage
			this.write32(0x0b12);						// biXPelsPerMeter
			this.write32(0x0b12);						// biYPelsPerMeter
			this.write32(0);							// biClrUsed
			this.write32(0);							// biClrImportant
		}
		else if (16 == this.depth) {
			if (width % 2)
				throw new Error("width must be multiple of 2");

			// 0x46 byte header
			this.file.write("BM");						// imageFileType
			this.write32((rowBytes * height) + 0x46);	// fileSize
			this.write16(0);							// reserved1
			this.write16(0);							// reserved2
			this.write32(0x46);							// imageDataOffset

			this.write32(0x38);							// biSize
			this.write32(width);						// biWidth
			this.write32(-height);						// biHeight (negative, because we write top-to-bottom)
			this.write16(1);							// biPlanes
			this.write16(16);							// biBitCount
			this.write32(3);							// biCompression (3 == 565 pixels (see mask below), 0 == 555 pixels)
			this.write32((rowBytes * height) + 2);		// biSizeImage
			this.write32(0x0b12);						// biXPelsPerMeter
			this.write32(0x0b12);						// biYPelsPerMeter
			this.write32(0);							// biClrUsed
			this.write32(0);							// biClrImportant

			if (Bitmap.ARGB4444 == this.pixelFormat)
				this.file.write(0x00, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00);	// masks for RGBA444 pixels
			else
				this.file.write(0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);	// masks for 565 pixels
		}
		else if (8 == this.depth) {
			if (width % 4)
				throw new Error("width must be multiple of 4");

			// 0x436 byte header
			this.file.write("BM");						// imageFileType
			this.write32((rowBytes * height) + 0x436);	// fileSize
			this.write16(0);							// reserved1
			this.write16(0);							// reserved2
			this.write32(0x436);						// imageDataOffset

			this.write32(0x28);							// biSize
			this.write32(width);						// biWidth
			this.write32(-height);						// biHeight (negative, because we write top-to-bottom)
			this.write16(1);							// biPlanes
			this.write16(8);							// biBitCount
			this.write32(0);							// biCompression (3 == 565 pixels (see mask below), 0 == 555 pixels)
			this.write32((rowBytes * height) + 2);		// biSizeImage
			this.write32(0x0b12);						// biXPelsPerMeter
			this.write32(0x0b12);						// biYPelsPerMeter
			this.write32(0);							// biClrUsed
			this.write32(0);							// biClrImportant

			if (Bitmap.Gray256 == this.pixelFormat) {
				for (let i = 0; i < 256; i++)
					this.file.write(i, i, i, 0);
			}
			else if (Bitmap.RGB332 == this.pixelFormat) {
				for (let i = 0; i < 256; i++) {
					let r = i >> 5;
					let g = (i >> 2) & 7;
					let b = i & 3;
					this.file.write((b << 6) | (b << 4) | (b << 2) | b, (g << 5) | (g << 2) | (g >> 1),
								(r << 5) | (r << 2) | (r >> 1), 0);
				}
			}
		}
		else if (4 == this.depth) {
			if (width % 8)
				throw new Error("width must be multiple of 8");

			// 0x76 byte header
			this.file.write("BM");						// imageFileType
			this.write32((rowBytes * height) + 0x76);	// fileSize
			this.write16(0);							// reserved1
			this.write16(0);							// reserved2
			this.write32(0x76);							// imageDataOffset

			this.write32(0x28);							// biSize
			this.write32(width);						// biWidth
			this.write32(-height);						// biHeight (negative, because we write top-to-bottom)
			this.write16(1);							// biPlanes
			this.write16(4);							// biBitCount
			this.write32(0);							// biCompression (3 == 565 pixels (see mask below), 0 == 555 pixels)
			this.write32((rowBytes * height) + 2);		// biSizeImage
			this.write32(0x0b12);						// biXPelsPerMeter
			this.write32(0x0b12);						// biYPelsPerMeter
			this.write32(0);							// biClrUsed
			this.write32(0);							// biClrImportant

			if (Bitmap.Gray16 == this.pixelFormat) {
				for (let i = 0; i < 16; i++) {
					let j = (i << 4) | i;
					this.file.write(j, j, j, 0);
				}
			}
			else if (Bitmap.CLUT16 == this.pixelFormat) {
				if (!this.colors)
					throw new Error("color table missing");

				for (let i = 0, clut = new Uint16Array(this.colors), j = 0; i < 16; i++, j += 4) {
					let entry = clut[i];
					let r = entry >> 8, g = ((entry >> 4) & 0x0F), b = entry & 0x0F;
					this.file.write((b << 4) | b, (g << 4) | g, (r << 4) | r, 0);
				}
			}
			else
				throw new Error("unsupported input pixel format");
		}
		else if (1 == this.depth) {
			if (width % 32)
				throw new Error("width must be multiple of 32");
			
			this.file.write("BM");						// imageFileType
			this.write32((rowBytes * height) + 0x3E);	// fileSize
			this.write16(0);							// reserved1
			this.write16(0);							// reserved2
			this.write32(0x3E);							// imageDataOffset

			this.write32(0x28);							// biSize
			this.write32(width);						// biWidth
			this.write32(-height);						// biHeight (negative, because we write top-to-bottom)
			this.write16(1);							// biPlanes
			this.write16(1);							// biBitCount
			this.write32(0);							// biCompression (3 == 565 pixels (see mask below), 0 == 555 pixels)
			this.write32((rowBytes * height) + 2);		// biSizeImage
			this.write32(0x0b12);						// biXPelsPerMeter
			this.write32(0x0b12);						// biYPelsPerMeter
			this.write32(0);							// biClrUsed
			this.write32(0);							// biClrImportant
			
			this.file.write(255, 255, 255, 0);
			this.file.write(0, 0, 0, 0);
		}
		else
			throw new Error("unsupported depth");
	}

	send(pixels, offset = 0, count = pixels.byteLength - offset) {
		if ((0 == offset) && (count == pixels.byteLength) && (pixels instanceof ArrayBuffer))		//@@ file.write should support HostBuffer
			this.file.write(pixels);
		else {
			let bytes = new Uint8Array(pixels);
			while (count--)
				this.file.write(bytes[offset++]);
		}
	}
	end() {
		this.file.close();
		delete this.file;
	}

	write16(value) {
		this.file.write(value & 0xff, (value >> 8) & 0xff);
	}
	write32(value) {
		this.file.write(value & 0xff, (value >> 8) & 0xff, (value >> 16) & 0xff, (value >> 24) & 0xff);
	}
}

Object.freeze(BMPOut);
