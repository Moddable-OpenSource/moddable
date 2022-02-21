/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import { FILE, TOOL } from "tool";
import Bitmap from "commodetto/Bitmap";
import parseBMP from "commodetto/ParseBMP";
import parseBMF from "commodetto/ParseBMF";
import RLE4Out from "commodetto/RLE4Out";
import Converter from "commodetto/Convert";

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.fontPath = null;
		this.bitmapPath = null;
		this.outputDirectory = null;
		this.rotation = 0;
		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {
			case "-i":
				argi++;	
				if (argi >= argc)
					throw new Error("-i: no bitmap!");
				name = argv[argi];
				if (this.bitmapPath)
					throw new Error("-i '" + name + "': too many bitmaps!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("-i '" + name + "': bitmap not found!");
				this.bitmapPath = path;
				break;
				
			case "-o":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no directory!");
				name = argv[argi];
				if (this.outputDirectory)
					throw new Error("-o '" + name + "': too many directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': directory not found!");
				this.outputDirectory = path;
				break;

			case "-r":
				argi++;	
				if (argi >= argc)
					throw new Error("-r: no rotation!");
				name = parseInt(argv[argi]);
				if ((name != 0) && (name != 90) && (name != 180) && (name != 270))
					throw new Error("-r: " + name + ": invalid rotation!");
				this.rotation = name;
				break;

			default:
				name = argv[argi];
				if (this.fontPath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.fontPath = path;
				break;
			}
		}
		if (!this.fontPath)
			throw new Error("no file!");
		let parts = this.splitPath(this.fontPath);
		if (!this.bitmapPath) {
			parts.extension = ".bmp"
			this.bitmapPath = this.joinPath(parts);
		}
		parts.extension = ".bf4";
		if (this.outputDirectory)
			parts.directory = this.outputDirectory;
		this.outputPath = this.joinPath(parts);
	}

	run() {
		let bmf = parseBMF(this.readFileBuffer(this.fontPath), true);
		let bytes = new Uint8Array(bmf);
		bytes.position = bmf.position + 4;
		bmf.writePosition = bmf.byteLength;

		let bmp = parseBMP(this.readFileBuffer(this.bitmapPath));
		let bmpIs4 = 4 == Bitmap.depth(bmp.pixelFormat);

		let output = new FILE(this.outputPath, "wb");

		let convert = new Converter(bmp.pixelFormat, Bitmap.Gray16);
		let chars = new Array(bmf.charCount);

		let sourceRowBytes = ((bmp.width * Bitmap.depth(bmp.pixelFormat)) + 7) >> 3;

		/*
			set our extra flags in the chnl field of the first glyph
		*/
		let position = bytes.position;
		let prev = this.readU32(bytes);
		let continuous = true, ascending = true;
		for (let char = 1; char < bmf.charCount; char++) {
			bytes.position = position + (20 * char);
			let c = this.readU32(bytes);
			if (c != (prev + 1))
				continuous = false;
			if (c <= prev)
				ascending = false;
			bytes.position = position + (20 * (char + 1));
			prev = c;
		}
		bytes.position = position;
		if (!continuous && !ascending)
			throw new Error("character codes must be ascending");
		bytes[position + 19] |= 0x80;
		if (continuous)
			bytes[position + 19] |= 0x40;
		if (ascending)
			bytes[position + 19] |= 0x20;

		bytes[position + 19] |= 0x10;			// kerning table sorted (below)

		/*
			compress each glyph independently
		*/
		for (let char = 0; char < bmf.charCount; char += 1, bytes.position += 8) {
			let dataOffset = bmf.writePosition - bytes.position;		// byte offset from characters entry
			let charCode = this.readU32(bytes);
			let sx = this.readS16(bytes);
			let sy = this.readS16(bytes);
			let sw = this.readS16(bytes);
			let sh = this.readS16(bytes);
			let tc, td;

			switch (this.rotation) {
				case 90:
					tc = sx;
					td = sw;
					sx = bmp.width - sy - sh;
					sy = tc;
					sw = sh;
					sh = td;
					break;
				case 270:
					tc = sx;
					td = sw;
					sx = sy
					sy = bmp.height - tc - sw;
					sw = sh;
					sh = td;
					break;
				case 180:
					sx = bmp.width - sx - sw;
					sy = bmp.height - sy - sh;
					break;
			}

			let sourceLineBytes = ((Bitmap.depth(bmp.pixelFormat) * sw) + 7) >> 3;
			if (bmpIs4 && (sx & 1) && !(sw & 1))
				sourceLineBytes += 1;
			let destLineBytes = ((Bitmap.depth(Bitmap.Gray16) * sw) + 7) >> 3;
			let destLine = (new Uint8Array(destLineBytes)).buffer;

			let writer = new RLE4Out({width: sw, height: sh, pixelFormat: Bitmap.Gray16});
			writer.begin(0, 0, sw, sh);

			let offset = bmp.offset + (sourceRowBytes * sy) + ((Bitmap.depth(bmp.pixelFormat) * sx) >> 3);
			for (let i = 0; i < sh; i++, offset += sourceRowBytes) {
				let sourceLine = bmp.buffer.slice(offset, offset + sourceLineBytes);
				if (bmpIs4) {
					if (sx & 1) {
						let slb = new Uint8Array(sourceLine);
						for (let j = 0; j < sourceLineBytes - 1; j++)
							slb[j] = (slb[j] << 4) | (slb[j + 1] >> 4);
						slb[sourceLineBytes - 1] = (slb[sourceLineBytes - 1] << 4) | 0x0F;
						writer.send(sourceLine, 0, (sw + 1) >> 1);
					}
					else
						writer.send(sourceLine);
				}
				else {
					convert.process(sourceLine, destLine);
					writer.send(destLine);
				}
			}

			writer.end();

			chars[char] = writer.bitmap.buffer;

			// overwrite sx and sy (not needed with compressed glyphs)
			bytes[bytes.position - 8] = (dataOffset >>  0) & 0xFF;
			bytes[bytes.position - 7] = (dataOffset >>  8) & 0xFF;
			bytes[bytes.position - 6] = (dataOffset >> 16) & 0xFF;
			bytes[bytes.position - 5] = (dataOffset >> 24) & 0xFF;

			bmf.writePosition += writer.bitmap.buffer.byteLength;
		}

		/*
			sort kern table
		*/
		if (5 == this.readU8(bytes)) {
			let count = this.readU32(bytes) / 10;
			let table = new DataView(bytes.buffer, bytes.position);

			// load table
			let kerns = new Array(count);
			for (let i = 0; i < count; i++) {
				let first = this.readU32(bytes);
				let second = this.readU32(bytes);
				let dx = this.readS16(bytes);
				kerns[i] = {first, second, dx};
			}

			// sort it
			kerns.sort((k1, k2) => {
				if (k1.first < k2.first)
					return -1;
				if (k1.first > k2.first)
					return +1;
				return k1.second - k2.second;
			});

			// write it back to BMF
			let position = 0;
			kerns.forEach(kern => {
				table.setUint32(position, kern.first, true)
				table.setUint32(position + 4, kern.second, true)
				table.setInt16(position + 8, kern.dx, true)
				position += 10;
			});
		}

		bytes[3] = 4;		// bump version number to 4 to indicate RLE4 compressed bitmap data
		output.writeBuffer(bmf);
		for (let char = 0; char < bmf.charCount; char++)
			output.writeBuffer(chars[char]);

		output.close();
	}

	readU32(bytes) {
		let value = bytes[bytes.position] |  (bytes[bytes.position + 1] << 8) | (bytes[bytes.position + 2] << 16) | (bytes[bytes.position + 3] << 24);
		bytes.position += 4;
		return value;
	}
	readS16(bytes) {
		let value = (bytes[bytes.position]) | (bytes[bytes.position + 1] << 8);
		if (value > 32767) value -= 65536;
		bytes.position += 2;
		return value;
	}
	readU8(bytes) {
		return bytes[bytes.position++];
	}
	readUTF8(bytes) {
		let c = this.readU8(bytes);

		if (!(c & 0x80))
			return c;

		let result;
		if (0xC0 == (c & 0xE0)) { // 2 byte sequence
			result = ((c & 0x1F) << 6) | (this.readU8(bytes) & 0x3F);
		}
		else if (0xE0 == (c & 0xF0)) { // 3 byte sequence
			result = ((c & 0x0F) << 12) | ((this.readU8(bytes) & 0x3F) << 6);
			result |= (this.readU8(bytes) & 0x3F);
		}
		else if (0xF0 == (c & 0xF1)) { // 4 byte sequence
			result = ((c & 0x0F) << 18) | ((this.readU8(bytes) & 0x3F) << 12);
			result |= (this.readU8(bytes) & 0x3F) << 6;
			result |= (this.readU8(bytes) & 0x3F);
		}
		else
			result = 0;

		return result;
	}
}
