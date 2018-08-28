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
	Commodetto BMF loader

		parses BMF data structure
		puts needed header fields and glyph metrics into ArrayBuffer for use by native rendering code

		http://www.angelcode.com/products/bmfont/doc/file_format.html


		N.B. version 4 is a Moddable variation which stores 4-bit compressed gray bitmaps
			in the file rather than referencing an externa; bitmap. x & y fields are replaced with a 32-bit
			little-endian offset to the compressed glyph (offset is from start of character code entry,
			not the start of the file)

*/

function parse() @ "xs_parseBMF"

export default parse;

/*
export default function parseBMF(font)
{
	let parser = new Parser(font);
	return parser.parse();
}

class Parser {
	constructor(buffer) {
		this.bytes = new Uint8Array(buffer);
	}

	seekTo(position) {
		this.position = position;
	}
	seekBy(delta) {
		this.position += delta;
	}
	readU32() {
		let value = this.bytes[this.position] |  (this.bytes[this.position + 1] << 8) | (this.bytes[this.position + 2] << 8) | (this.bytes[this.position + 3] << 8);
		this.position += 4;
		return value;
	}
	readS16() {
		let value = (this.bytes[this.position]) | (this.bytes[this.position + 1] << 8);
		if (value > 32767) value -= 65536;
		this.position += 2;
		return value;
	}
	readU8() {
		return this.bytes[this.position++];
	}

	parse() {
		let bytes = this.bytes, buffer = bytes.buffer;

		if ((0x42 != bytes[0]) || (0x4D != bytes[1]) || (0x46 != bytes[2]) || ((3 != bytes[3]) && (4 != bytes[3])))
			throw new Error("Invalid BMF header");

		this.seekTo(4);

		// skip block 1
		if (1 != this.readU8())
			throw new Error("can't find info block");

		this.seekBy(this.readU32());

		// get lineHeight from block 2
		if (2 != this.readU8())
			throw new Error("can't find common block");
		let size = this.readU32();

		buffer.height = this.readS16();
		buffer.ascent = this.readS16();
		this.seekBy(size - 4);

		// skip block 3
		if (3 != this.readU8())
			throw new Error("can't find pages block");

		this.seekBy(this.readU32());

		// use block 4
		if (4 != this.readU8())
			throw new Error("can't find chars block");

		buffer.position = this.position;		// position of size of chars table

		size = this.readU32();
		if (size % 20)
			throw new Error("bad chars block size");
//@@ this scan of the character table should be unnecessary on device... font data has already checked at buildtime
		let firstChar = this.readU32();
		let lastChar = firstChar + (size / 20);
		this.seekBy(20 - 4);
		for (let i = firstChar + 1; i < lastChar; i++) {
			let c = this.readU32();
			if (c != i)
				throw new Error("gap detected - chars must be consecutive");
			this.seekBy(20 - 4);
		}

		buffer.firstChar = firstChar;
		buffer.lastChar = lastChar;

		return buffer;
	}
};

Object.freeze(Parser.prototype);

*/
