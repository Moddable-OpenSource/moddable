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
	Commodetto NFNT loader
	
		parses NFNT data structure
		puts needed header fields and glyph metrics into ArrayBuffer for use by native rendering code
		creates monochrome bitmap object (header.bitmap) referencing the glyph bitmaps

*/

import Bitmap from "Bitmap";

export default function parseNFNT(font)
{
	let parser = new Parser(font);
	return parser.parse();
}

class Parser {
	constructor(buffer) {
		this.buffer = buffer;
		this.bytes = new Uint8Array(buffer);
		this.position = undefined;
	}

	seekTo(position) {
		this.position = position;
	}
	seekBy(delta) {
		this.seekTo(this.position + delta);
	}
	readS16() {
		var value = (this.bytes[this.position] << 8) | this.bytes[this.position + 1];
		if (value > 32767) value -= 65536;
		this.position += 2;
		return value;
	}
	readS8() {
		var value = this.bytes[this.position++];
		if (value > 127) value -= 256;
		return value;
	}
	readU8() {
		return this.bytes[this.position++];
	}

	parse() {
		this.seekTo(0);

	/*	let fontType = */ this.readS16();			// unused
		let firstChar = this.readS16();
		let lastChar = this.readS16();
	/*	let maxWidth = */ this.readS16();			// unused
		let maxKern = this.readS16();
	/*	let descent = */ -this.readS16();			// unused
		let width = this.readS16();
		let height = this.readS16();	// (fontRectHeight)
		let widthTablePosition = (this.readS16() * 2) + 16;
	/*	let maxAscent = */ this.readS16();			// unused
	/*	let maxDescent = */ this.readS16();			// unused
		let leading = this.readS16();
		let rowWords = this.readS16();

		let bitmapSize = rowWords * 2 * height;

		let charCount = lastChar - firstChar + 1;
		let info = new ArrayBuffer(16 + (4 * charCount));	// 16 bytes for font header

		let header = new DataView(info, 0, 16);	// only write into header
		// write into binary header for native code
		let endian = true;		// little-endian //@@ base on host
		header.setInt16(0, firstChar, endian);
		header.setInt16(2, lastChar, endian);
		header.setInt16(4, maxKern, endian);
		header.setInt16(6, height, endian);
		header.setInt16(8, leading, endian);
		header.setInt16(10, rowWords, endian);
		header.setInt32(12, widthTablePosition, endian);
		header.bitmap = new Bitmap(rowWords * 16, height, Bitmap.Monochrome, this.buffer, 26);

		// write into JavaScript object for scripts (may be able to remove this)
		header.firstChar = firstChar;
		header.lastChar = lastChar;
		header.maxKern = maxKern;
		header.height = height;
		header.leading = leading;

		this.seekBy(bitmapSize);				// skip over "Bit image table"

		header.glyphs = info;

		let glyphs = new DataView(info, 16);	// glyph table follows header

		// read "Bitmap location table"
		for (let i = 0, position = 0; i < charCount; i++, position += 4)
			glyphs.setInt16(position, this.readS16(), endian);

		this.seekTo(widthTablePosition);

		// read "width offset table"
		for (let i = 0, position = 2; i < charCount; i++, position += 4) {
			glyphs.setInt8(position, this.readS8());			// xOffset (signed)
			glyphs.setInt8(position + 1, this.readU8());		// width (unsigned)
		}

		// info buffer now contains a byte array, one entry per glyph starting at firstChar
		// SInt16 - location, SInt8 xOffset, UInt8 width

		return header;
	}
};

Object.freeze(Parser.prototype);
