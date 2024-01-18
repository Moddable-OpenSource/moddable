/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.sourcePath = null;
		this.outputDirectory = null;
		this.outputName = null;
		this.address = 0;
		this.currentSegment = 0;
		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {				
			case "-n":
				argi++;	
				if (argi >= argc)
					throw new Error("-n: no name!");
				name = argv[argi];
				if (this.outputName)
					throw new Error("-n '" + name + "': too many output names!");
				this.outputName = name;
				break;

			case "-o":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no path!");
				name = argv[argi];
				if (this.outputDirectory)
					throw new Error("-o '" + name + "': too many output paths!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': path not found!");
				this.outputDirectory = path;
				break;

			case "-a":
				argi++;
				if (argi >= argc)
					throw new Error("-a: no value!");
				this.address = parseInt(argv[argi]);
				break;

			default:
				name = argv[argi];
				if (this.sourcePath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.sourcePath = path;
				break;
			}
		}
		if (!this.sourcePath)
			throw new Error("no input file!");

		if (this.address == 0)
			throw new Error("-a: bad starting address: " + this.address);
	}

	hexChecksum(s) {
		if (s[0] != ':')
			throw new Error("checksum for '" + s + "' failed: no initial :");

		let accum = 0;
		for (let i=0; i<(s.length-1)/2; i++) {
			let chunk = s.slice(i*2+1, i*2+3);
			accum += parseInt(chunk, 16);
		}

		let twoC = ((~(accum-1)) & 0xff);
		return ("00" + twoC.toString(16)).slice(-2).toUpperCase();
	}

	generateSegment(addr) {
		this.currentSegment = (addr & 0xffff0000);
		let out = ":02000002" + ("0000" + (this.currentSegment>>4).toString(16)).slice(-4);
		let chk = this.hexChecksum(out);
		return (out + chk).toUpperCase();
	}

	run() {
		const source = new Uint8Array(this.readFileBuffer(this.sourcePath));
		let len = source.length;
		let pos = 0;
		let address = this.address;

		const parts = this.splitPath(this.sourcePath);
		parts.extension = ".hex";
		if (this.outputDirectory)
			parts.directory = this.outputDirectory;
		if (this.outputName)
			parts.name = this.outputName; 
		const outputPath = this.joinPath(parts);

		const output = new FILE(outputPath, "wb");

		while (len) {
			if (this.currentSegment != (address & 0xffff0000))
				output.writeString(this.generateSegment(address) + "\n");

			const lineSize = len > 16 ? 16 : len;

			const buf = source.slice(pos, pos+lineSize);
			pos += lineSize;
			len -= lineSize;

			let out = ":" + ("00" + lineSize.toString(16)).slice(-2);
			out += ("0000" + (address - this.currentSegment).toString(16)).slice(-4);
			out += "00";
			for(let j=0; j<lineSize; j++)
				out += ("00" + (buf[j].toString(16))).slice(-2);

			const chk = this.hexChecksum(out);
			output.writeString((out + chk + "\n").toUpperCase());

			address += lineSize;
		}

		output.writeString(":00000001FF");
		output.close();
	}
}
