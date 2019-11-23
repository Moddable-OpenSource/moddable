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

var platformNames = {
	mac: "mac",
	macosx: "mac",
	MacOSX: "mac",
};

var types = {
	".png": "PNG",
	".ttf": "TTF",
};

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.name = null;
		this.outputPath = null;
		this.resources = [];
		this.signature = null;
		this.windows = this.currentPlatform == "win";
		var argc = argv.length;
		var name, path;
		for (var argi = 1; argi < argc; argi++) {
			var option = argv[argi];
			switch (option) {
			case "-r":
				argi++;	
				if (argi >= argc)
					throw new Error("-r: no name!");
				name = argv[argi];
				if (this.name)
					throw new Error("-r '" + name + "': too many names!");
				this.name = name;
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
				this.outputPath = path;
				break;
			case "-p":
				argi++;	
				if (argi >= argc)
					throw new Error("-p: no platform!");
				name = argv[argi];
				if (this.platform)
					throw new Error("-p '" + name + "': too many platforms!");
				if (name in platformNames)
					name = platformNames[name];
				this.platform = name;
				break;
			case "-s":
				argi++;	
				if (argi >= argc)
					throw new Error("-s: no signature!");
				name = argv[argi];
				if (this.signature)
					throw new Error("-s '" + name + "': too many signatures!");
				this.signature = name;
				break;
			default:
				name = argv[argi];
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': resource not found!");
				this.resources.push(path);
				break;
			}
		}
		if (!this.name)
			this.name = "mc.resources.c";
		if (!this.outputPath)
			this.outputPath = this.currentDirectory;
		if (!this.platform)
			this.platform = this.currentPlatform;
	}
	run() {
		var path = this.joinPath({directory: this.outputPath, name:this.name});
		var file = new FILE(path);
		if (this.platform == "x-lin") {
			var length = this.outputPath.length + 1;
			var signature = this.signature;
			if (!signature)
				throw new Error("no signature!");
			file.line('<?xml version="1.0" encoding="UTF-8"?>');
			file.line('<gresources>');
			file.line('\t<gresource prefix="', signature, '">');
			this.resources.forEach((path, index) => {
				//let fileParts = this.splitPath(path);
				file.line('\t\t<file>', path.slice(length), '</file>');
			});
			file.line('\t</gresource>');
			file.line('</gresources>');
		}
		else if (this.platform == "x-win") {
			file.line("#include \"windows.h\"");
			this.resources.forEach((path, index) => {
				let fileParts = this.splitPath(path);
				let directoryParts = this.splitPath(fileParts.directory);
				let type = types[fileParts.extension];
				if (type == "TTF")
					file.line(fileParts.name, " ", type, " DISCARDABLE \".\\\\resources\\\\", directoryParts.name, "\\\\", fileParts.name, fileParts.extension, "\"");
				else
					file.line(directoryParts.name, "/", fileParts.name, fileParts.extension, " ", type, " DISCARDABLE \".\\\\resources\\\\", directoryParts.name, "\\\\", fileParts.name, fileParts.extension, "\"");
			});
		}
		else {
			file.line("#include \"xsPlatform.h\"");
			this.resources.forEach((path, index) => {
				if (("win" === this.platform) || ("x-win" === this.platform))
					file.line("static const unsigned char _", index, "[] = {");
				else
					file.line("static const unsigned char _", index, "[] __attribute__((aligned(4))) = {");
				file.dump(path);
				file.line("};");
			});
			file.line("static const struct {const char *name; const unsigned char *data; unsigned int size;} resources[] = {");
			if (this.resources.length) {
				this.resources.forEach((path, index) => {
					var parts = this.splitPath(path);
					file.line("\t{\"", parts.name, parts.extension, "\", _", index, ", sizeof(_", index, ")},");
				});
			}
			else {
				file.line("\t{ NULL, NULL, 0 },");
			}
			file.line("};");
			file.line("extern const void *mcGetResource(void* it, const char *path, size_t *sizep);");
			file.line("const void *mcGetResource(void* it, const char *path, size_t *sizep)");
			file.line("{");
			if (this.resources.length) {
				file.line("\tunsigned int i;");
				file.line("\t");
				file.line("\tfor (i = 0; i < ", this.resources.length, "; i++) {");
				file.line("\t\tif (c_strcmp(resources[i].name, path) == 0) {");
				file.line("\t\t\t*sizep = resources[i].size;");
				file.line("\t\t\treturn resources[i].data;");
				file.line("\t\t}");
				file.line("\t}");
			}
			file.line("\treturn NULL;");
			file.line("}");
			file.line("extern int mcCountResources(void* it);");
			file.line("int mcCountResources(void* it)");
			file.line("{");
			file.line("\treturn ", this.resources.length, ";");
			file.line("}");
			file.line("extern const char *mcGetResourceName(void* it, int i);");
			file.line("const char *mcGetResourceName(void* it, int i)");
			file.line("{");
			file.line("\treturn resources[i].name;");
			file.line("}");
		}
		file.close();
		let total = this.resources.reduce((total, path) => total + this.getFileSize(path), 0);
		this.report(`Total resource size: ${total} bytes`);
	}
}
