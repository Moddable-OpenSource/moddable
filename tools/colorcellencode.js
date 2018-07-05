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

import * as FS from "fs";
import TOOL from "tool";
import Bitmap from "commodetto/Bitmap";
import parseBMP from "commodetto/ParseBMP";
import ColorCellOut from "commodetto/ColorCellOut";
import Converter from "commodetto/Convert";

export default class Tool extends TOOL {
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
		parts.extension = ".cs";
		let output = FS.openSync(this.joinPath(parts), "wb");

		let uncompressed = parseBMP(FS.readFileBufferSync(this.inputPath));
		let convert = new Converter(uncompressed.pixelFormat, Bitmap.RGB565LE);

		let writer = new ColorCellOut;
		writer.begin(0, 0, uncompressed.width, uncompressed.height);

		const cellHeight = 4;
		let sourceLineBytes = ((Bitmap.depth(uncompressed.pixelFormat) * uncompressed.width) + 7) >> 3;
		let destLineBytes = uncompressed.width * 2;
		let destBuffer = (new Uint8Array(cellHeight * destLineBytes)).buffer;
		for (let i = 0, offset = uncompressed.offset; i < uncompressed.height; i += cellHeight) {
			let sourceLine = uncompressed.buffer.slice(offset + (i * sourceLineBytes), offset + ((i + cellHeight) * sourceLineBytes));
			convert.process(sourceLine, destBuffer);
			writer.send(destBuffer);
		}

		writer.end();

		let compressed = writer.bitmap;

		// CommodettoStream header
		const frameCount = 1;
		const fps_numerator = 1;
		const fps_denominator = 1;
		const frameSize = compressed.buffer.byteLength;

		FS.writeByteSync(output, 'c'.charCodeAt(0));
		FS.writeByteSync(output, 's'.charCodeAt(0));
		FS.writeByteSync(output, Bitmap.RGB565LE);
		FS.writeByteSync(output, 0);
		FS.writeByteSync(output, compressed.width & 0xFF);
		FS.writeByteSync(output, compressed.width >> 8);
		FS.writeByteSync(output, compressed.height & 0xFF);
		FS.writeByteSync(output, compressed.height >> 8);
		FS.writeByteSync(output, frameCount & 0xFF);
		FS.writeByteSync(output, frameCount >> 8);
		FS.writeByteSync(output, fps_numerator & 0xFF);
		FS.writeByteSync(output, fps_numerator >> 8);
		FS.writeByteSync(output, fps_denominator & 0xFF);
		FS.writeByteSync(output, fps_denominator >> 8);

		FS.writeByteSync(output, frameSize & 255);
		FS.writeByteSync(output, frameSize >> 8);
		FS.writeBufferSync(output, compressed.buffer);

		FS.closeSync(output);
	}
}
