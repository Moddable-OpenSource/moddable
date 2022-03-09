/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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
import compileDataView from "compileDataView"

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.sourcePath = null;
		this.outputDirectory = null;
		this.outputName = null;
		this.pragmas = {};
		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {				
			case "-n":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no name!");
				name = argv[argi];
				if (this.outputName)
					throw new Error("-o '" + name + "': too many output names!");
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

			case "-p":
				argi++;	
				if (argi >= argc)
					throw new Error("-p: no value!");
				name = argv[argi];
				name = name.split("=");
				if (2 !== name.length)
					throw new Error("-p '" + name + "': bad value!");
				this.pragmas[name[0]] = name[1];
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
		this.language ??= "javascript";
	}

	run() {
		const source = this.readFileString(this.sourcePath);
		const result = compileDataView(source, this.pragmas);
		
		if (result.errors) {
			this.report(result.errors);
			throw new Error("cdv failed for " + this.sourcePath);
		}			

		const parts = this.splitPath(this.sourcePath);
		parts.extension = "." + result.language;
		if (this.outputDirectory)
			parts.directory = this.outputDirectory;
		if (this.outputName)
			parts.name = this.outputName; 
		const outputPath = this.joinPath(parts);

		const output = new FILE(outputPath, "wb");
		output.writeString(result.script);
		output.close();
	}
}
