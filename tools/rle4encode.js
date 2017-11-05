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
import RLE4Out from "commodetto/RLE4Out";
import Converter from "commodetto/Convert";

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.inputPath = null;
		this.outputPath = null;
		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {
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
				this.outputPath = path;
				break;

			default:
				name = argv[argi];
				if (this.inputPath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.inputPath = path;
				break;
			}
		}
		if (!this.inputPath) {
			throw new Error("no file!");
		}
		if (!this.outputPath)
			this.outputPath = this.splitPath(this.inputPath).directory;
	}

	run() {
		let parts = this.splitPath(this.inputPath);
		parts.directory = this.outputPath;
		parts.extension = ".bm4";
		let output = new FILE(this.joinPath(parts), "wb");

		let uncompressed = parseBMP(this.readFileBuffer(this.inputPath));
		let convert = new Converter(uncompressed.pixelFormat, Bitmap.Gray16);

		let writer = new RLE4Out({width: uncompressed.width, height: uncompressed.height, pixelFormat: Bitmap.Gray16});
		writer.begin(0, 0, uncompressed.width, uncompressed.height);

		let sourceLineBytes = ((Bitmap.depth(uncompressed.pixelFormat) * uncompressed.width) + 7) >> 3;
		let destLineBytes = ((4 * uncompressed.width) + 7) >> 3;
		let destLine = (new Uint8Array(destLineBytes)).buffer;
		for (let i = 0, offset = uncompressed.offset; i < uncompressed.height; i++) {
			let sourceLine = uncompressed.buffer.slice(offset + (i * sourceLineBytes), offset + ((i + 1) * sourceLineBytes));
			convert.process(sourceLine, destLine);
			writer.send(destLine);
		}

		writer.end();

		let compressed = writer.bitmap;

		// header: 'md', version, Commodetto pixel format, width and height (big endian)
		output.writeByte('m'.charCodeAt(0));
		output.writeByte('d'.charCodeAt(0));
		output.writeByte(0);
		output.writeByte(compressed.pixelFormat);
		output.writeByte(compressed.width >> 8);
		output.writeByte(compressed.width & 0xFF);
		output.writeByte(compressed.height >> 8);
		output.writeByte(compressed.height & 0xFF);

		output.writeBuffer(compressed.buffer);

		output.close();
	}
}
