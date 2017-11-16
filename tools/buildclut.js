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

/*
	Photoshop colortable (.act) is 8-bit RGB triples.
	
	Take first 16 as input
	
	 Output is:
	 
		4:4:4 pixel for each color (16 2-byte little endian pixels)
		Remap table: Maps 4:4:4 pixel to 8-bit index (only 4 bits used) - 4096 bytes
*/

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
		parts.extension = ".cct";
		let output = new FILE(this.joinPath(parts), "wb");

		let photoshopCLUT = new Uint8Array(this.readFileBuffer(this.inputPath));

		// 4:4:4 color table
		for (let i = 0, c = 0; i < 16; i++, c += 3) {
			output.writeByte((this.roundComponent(photoshopCLUT[c + 1]) & 0xF0) |
										(this.roundComponent(photoshopCLUT[c + 2]) >> 4));
			output.writeByte(this.roundComponent(photoshopCLUT[c + 0]) >> 4);
		}

		// inverse table from 4:4:4 to 16 CLUT
		for (let r = 0; r < 16; r++) {
			for (let g = 0; g < 16; g++) {
				for (let b = 0; b < 16; b++) {
					let nearest = this.findNearest(photoshopCLUT, r | (r << 4), g | (g << 4), b | (b << 4));
					output.writeByte(nearest);
				}
			}
		}

		// 5:6:5 color table
		for (let i = 0, c = 0; i < 16; i++, c += 3) {
			let pixel = ((photoshopCLUT[c + 0] >> 3) << 11) | ((photoshopCLUT[c + 1] >> 2) << 5) | (photoshopCLUT[c + 2] >> 3);
			output.writeByte(pixel & 0xFF);
			output.writeByte(pixel >> 8);
		}

		output.close();
	}

	findNearest(clut, r, g, b) {
		let bestDelta = 1000000;
		let nearest;

		for (let i = 0, c = 0; i < 16; i++, c += 3) {
			let dr = clut[c + 0] - r;
			let dg = clut[c + 1] - g;
			let db = clut[c + 2] - b;
			let delta = Math.sqrt((dr * dr) + (dg * dg) + (db * db));
//			let delta = ((clut[c + 0] - r) ^ 2) + ((clut[c + 1] - g) ^ 2) + ((clut[c + 2] - b) ^ 2);
			if (delta < bestDelta) {
				nearest = i;
				bestDelta = delta;
			}
		}

		return nearest;
	}

	// prepare to truncate 8 bits to 4 bits
	roundComponent(c) {
		if (c >= 248) return c;
		return c + 7;
	}
}
