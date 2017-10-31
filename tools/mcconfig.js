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

import { File, MakeFile as MAKEFILE, PrerequisiteFile, FormatFile, RotationFile, Tool as TOOL } from "mcmanifest";
import * as FS from "fs";

var formatStrings = {
	gray16: "Gray16",
	gray256: "Gray256",
	rgb332: "RGB332",
	rgb565le: "RGB565LE",
	rgb565be: "RGB565BE",
	clut16: "CLUT16",
	x: "",
};

var formatValues = {
	gray16: 4,
	gray256: 5,
	rgb332: 6,
	rgb565le: 7,
	rgb565be: 8,
	clut16: 11,
	x: 0,
};

var resourceExtensions = [  
	".act", ".fnt", ".json", ".png", ".gif", ".jpg", ".jpeg", 
	".bmp", ".cct", ".nfnt", ".rle", ".ttf", ".zip", // copy
	".dat", ".der", ".pk8", ".ski", ".subject",
];

class MakeFile extends MAKEFILE {
	constructor(path) {
		super(path)
	}
	generateManifestDefinitions(tool) {
		var creation = tool.creation;
		this.line("CREATION = -c ", 
				creation.chunk.initial, ",", 
				creation.chunk.incremental, ",", 
				creation.heap.initial, ",", 
				creation.heap.incremental, ",", 
				creation.stack, ",", 
				creation.keys.available, ",", 
				creation.keys.name, ",", 
				creation.keys.symbol, ",",
				creation.static, ",", 
				creation.main);
		this.line("");
		this.write("MANIFEST =");
		for (var result in tool.manifests.already) {
			this.write(" \\\n\t");
			this.write(result);
		}	
		this.line("");
		this.write("STRIPS =");
		if (tool.strip) {
			for (var result of tool.strip) {
				this.write("\\\n\t-s \"");
				this.write(result);
				this.write("\"");
			}
		}
		this.line("");
		this.line("");
	}
	generateModulesDefinitions(tool) {
		this.write("MODULES =");
		for (var result of tool.jsFiles) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}	
		for (var result of tool.cFiles) {
			var sourceParts = tool.splitPath(result.source);
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write(sourceParts.name);
			this.write(".xsi");
		}
		this.line("");
		this.write("PRELOADS =");
		for (var result of tool.preloads) {
			this.write("\\\n\t-p ");
			this.write(result);
		}	
		this.line("");
		this.line("");
	}
	generateModulesRules(tool) {
		super.generateModulesRules(tool);
		for (var result of tool.cFiles) {
			var source = result.source;
			var sourceParts = tool.splitPath(result.source);
			this.line("$(TMP_DIR)", tool.slash, sourceParts.name, ".xsi: ", source);
			this.echo(tool, "xsid ", sourceParts.name, ".xsi");
			this.line("\t$(XSID) ", source, " -o $(@D)");
		}
		this.line("");
	}
	generateObjectsDefinitions(tool) {
		this.write("DIRECTORIES =");
		for (var folder of tool.cFolders) {
			this.write("\\\n\t-I");
			this.write(folder);
		}	
		this.line("");
		if (tool.format) {
			this.line("DISPLAY = ", formatValues[tool.format]);
			this.line("ROTATION = ", tool.rotation);
		}
		this.write("HEADERS =");
		for (var header of tool.hFiles) {
			this.write("\\\n\t");
			this.write(header);
		}	
		if (tool.format) {
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write("mc.defines.h");
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write("mc.format.h");
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write("mc.rotation.h");
		}
		this.line("");
		this.write("OBJECTS =");
		for (var result of tool.cFiles) {
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}	
		this.line("");
	}
	generateObjectsRules(tool) {
		for (var result of tool.cFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(TMP_DIR)/", target, ": ", source, " $(HEADERS) | $(TMP_DIR)/mc.xs.c");
			if (result.recipe) {
				this.write(tool.recipes[result.recipe]);
			}
			else {
				this.echo(tool, "cc ", target);
				this.line("\t$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@");
			}
		}
	}
}

class AndroidMakeFile extends MakeFile {
	constructor(path) {
		super(path)
	}
	generateObjectsDefinitions(tool) {
		this.line("SOURCES_DIR = ", tool.javaPath);
		this.write("SOURCES =");
		for (var result of tool.javaFiles) {
			var source = result.source;
			var buffer = FS.readFileSync(source);
			var a = buffer.match(/package ([^;]+);/m);
			if (a.length != 2) {
				throw new Error("no package in ", source);
			}
			a = a[1].split(".");
			var path = tool.javaPath;
			for (var name of a) {
				path += "/" + name;
				FS.mkdirSync(path);
			}			
			var parts = tool.splitPath(source);
			var target = a.join("/") + "/" + parts.name + ".java";
			this.write("\\\n\t$(SOURCES_DIR)/");
			this.write(target);
			result.target = target;
		}
		this.line("");
	}
	generateObjectsRules(tool) {
		for (var result of tool.javaFiles) {
			this.line("$(SOURCES_DIR)/", result.target, ": ", result.source);
			this.echo(tool, "copy ", result.target);
			this.line("\tcp $< $@");
		}
	}
}

class NMakeFile extends MakeFile {
	constructor(path) {
		super(path)
	}
	generateObjectsRules(tool) {
		for (var result of tool.cFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(TMP_DIR)\\", target, ": ", source, " $(HEADERS)");
			if (result.recipe) {
				var recipe = tool.recipes[result.recipe];
				recipe = recipe.replace(/\$</g, source);
				this.write(recipe);
			}
			else {
				this.echo(tool, "cl ", target);
				this.line("\tcl $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) ", source, " /Fo$@");
			}
		}
	}
}

class espNMakeFile extends NMakeFile {
	constructor(path) {
		super(path)
	}
	generateObjectsRules(tool) {
		for (var result of tool.cFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(TMP_DIR)\\", target, ": ", source, " $(HEADERS)");
			if (result.recipe) {
				var recipe = tool.recipes[result.recipe];
				recipe = recipe.replace(/\$</g, source);
				this.write(recipe);
			}
			else {
				this.echo(tool, "cc ", target);
				this.line("\t$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) ", source, " -o $@");
				this.line("\t$(AR) $(AR_OPTIONS) $(APP_ARCHIVE) $@");
			}
		}
	}
}

class SynergyNMakeFile extends NMakeFile {
	constructor(path) {
		super(path)
	}
	generateObjectsRules(tool) {
		for (var result of tool.cFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(TMP_DIR)\\", target, ": ", source, " $(HEADERS)");
			if (result.recipe) {
				var recipe = tool.recipes[result.recipe];
				recipe = recipe.replace(/\$</g, source);
				this.write(recipe);
			}
			else {
				this.echo(tool, "cc ", target);
				this.line("\t$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) ", source, " -o $@");
				this.line("\t$(AR) $(AR_OPTIONS) $(ARCHIVE_FILE) $@");
			}
		}
	}
}

class CMakeListsFile extends PrerequisiteFile {
	constructor(path) {
		super(path);
	}
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		this.line("cmake_minimum_required(VERSION 3.4.1)");
		this.line("");
		this.line("add_library(tech-moddable-piu SHARED");
		this.line("\t", tool.xsPath, "/platforms/android_xs.c");
		var names = FS.readDirSync(tool.xsPath + "/sources");
		var c = names.length;
		for (var i = 0; i < c; i++) {
			var name = names[i];
			if (name.endsWith(".c") && (name != "xsDefaults.c"))
				this.line("\t", tool.xsPath, "/sources/", name);
		}
		for (var result of tool.cFiles) {
			this.line("\t", result.source);
		}
		this.line("\t", tool.tmpPath, "/mc.xs.c");
		this.line(")");
		
		this.line("target_compile_definitions(tech-moddable-piu PUBLIC");
		this.line("\tINCLUDE_XSPLATFORM=1");
		this.line("\tXSPLATFORM=\"android_xs.h\"");
		this.line("\tmxDebug=1");
		this.line("\tmxRun=1");
		this.line("\tmxParse=1");
		this.line("\tmxNoFunctionLength=1");
		this.line("\tmxNoFunctionName=1");
		this.line("\tmxHostFunctionPrimitive=1");
		this.line("\tmxFewGlobalsTable=1");
		this.line(")");
		
		this.line("target_include_directories(tech-moddable-piu PUBLIC");
		this.line("\t", tool.xsPath, "/includes");
		this.line("\t", tool.xsPath, "/platforms");
		this.line("\t", tool.xsPath, "/sources");
		this.line("\t", tool.xsPath, "/sources/pcre");
		for (var folder of tool.cFolders) {
			this.line("\t", folder);
		}	
		this.line("\t", tool.tmpPath);
		this.line(")");

		this.line("target_link_libraries(tech-moddable-piu");
   		this.line("\tandroid");
		this.line("\tlog");
		this.line(")");

		this.close();
	}
}

class ConfigFile extends PrerequisiteFile {
	constructor(path) {
		super(path);
	}
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		this.write("export default Object.freeze(");
		this.write(JSON.stringify(tool.config, null, "\t"));
		this.line(");");
		this.close();
	}
}

class DefinesFile extends PrerequisiteFile {
	constructor(path) {
		super(path);
	}
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		this.generateDefines(tool, "MODDEF_", tool.defines)
		if (tool.platform == "x-lin") {
			this.line('#define PIU_DASH_SIGNATURE "', tool.environment.DASH_SIGNATURE, '"');
			this.line('#define PIU_DOT_SIGNATURE "', tool.environment.DOT_SIGNATURE, '"');
			this.line('#define PIU_SLASH_SIGNATURE "', tool.environment.SLASH_SIGNATURE, '"');
		}
		this.close();
	}
	generateDefines(tool, prefix, defines) {
		let name;

		for (name in defines) {
			let value = defines[name];
			name = name.toUpperCase();

			switch (typeof value) {
				case "number":
				case "boolean":
					this.line(`#define ${prefix + name} (${value})`);
					break;

				case "string":
					if ("#" == value.charAt(0))
						this.line(`#define ${prefix + name} "${value.substring(1)}"`);
					else
						this.line(`#define ${prefix + name} ${value}`);
					break;

				case "object":
					if (null === value)
						this.line(`#define ${prefix + name} NULL`);
					else if (Array.isArray(value)) {
						this.write(`#define ${prefix + name} {`);
						for (let i = 0, length = value.length; i < length; i++) {
							if (i)
								this.write(", ");
							if ("number" != typeof value[i])
								throw new Error("Array element not number!");
							this.write(value[i]);
						}
						this.write("}\n");
					}
					else
						this.generateDefines(tool, prefix + name + "_", value);
					break;
			}
		}
	}
}

export default class Tool extends TOOL {
	constructor(argv) {
		super(argv);
		var path = this.moddablePath + this.slash + "tools" + this.slash + "mcconfig" + this.slash;
		if (this.windows)
			path += "nmake.";
		else
			path += "make.";
		path += this.platform + ".mk";
		path = this.resolveFilePath(path);
		if (!path)
			throw new Error("unknown platform!");
		this.fragmentPath = path;
	}
	createDirectories(path, first, last) {
		FS.mkdirSync(path);
		path += this.slash + first;
		FS.mkdirSync(path);
		var platform = this.platform;
		if (platform.startsWith("x-")) {
			path += this.slash + platform.slice(2);
			FS.mkdirSync(path);
		}
		else {
			let parts = platform.split("/");
			path += this.slash + parts[0];
			FS.mkdirSync(path);
			if (parts.length > 1) {
				path += this.slash + parts[1];
				FS.mkdirSync(path);
			}
		}
		if (this.debug) 
			path += this.slash + "debug";
		else if (this.instrument) 
			path += this.slash + "instrument";
		else
			path += this.slash + "release";
		FS.mkdirSync(path);
		if ((platform == "mac") || (platform == "win") || (platform == "lin")) {
			path += this.slash + "mc";
			FS.mkdirSync(path);
		}
		if (last) {
			path += this.slash + last;
			FS.mkdirSync(path);
		}
		return path;
	}
	filterCommonjs(commonjs) {
		this.commonjs = false;
		if (commonjs.length) {
			this.commonjs = true;
			for (var jsFile of this.jsFiles) {
				jsFile.commonjs = false;
			}
			for (var pattern of commonjs) {
				pattern = this.resolveSlash(pattern);
				var star = pattern.lastIndexOf("*");
				if (star >= 0) {
					pattern = pattern.slice(0, star);
					for (var result of this.jsFiles) {
						var target = result.target.slice(0, star);
						if (target == pattern)
							result.commonjs = true;
					}
				}
				else {
					pattern += ".xsb";
					for (var result of this.jsFiles) {
						var target = result.target;
						if (target == pattern)
							result.commonjs = true;
					}
				}
			}
		}
	}
	filterConfig(config) {
		this.mergeProperties(config, this.config);
		if (this.format) {
			config.format = formatStrings[this.format];	
			config.rotation = this.rotation;
		}
		this.config = config;
	}
	filterCreation(creation) {
		if (!creation.chunk) creation.chunk = { };
		if (!creation.chunk.initial) creation.chunk.initial = 32768;
		if (!creation.chunk.incremental) creation.chunk.incremental = 1024;
		if (!creation.heap) creation.heap = { };
		if (!creation.heap.initial) creation.heap.initial = 2048;
		if (!creation.heap.incremental) creation.heap.incremental = 64;
		if (!creation.stack) creation.stack = 512;
		if (!creation.keys) creation.keys = {};
		if (!creation.keys.available) creation.keys.available = 256;
		if (!creation.keys.name) creation.keys.name = 127;
		if (!creation.keys.symbol) creation.keys.symbol = 127;
		if (!creation.static) creation.static = 0;
		if (!creation.main) creation.main = "main";
		if ((this.platform == "x-android") || (this.platform == "x-android-simulator") || (this.platform == "x-ios") || (this.platform == "x-ios-simulator")) {
			creation.main = this.ipAddress;
		}
		this.creation = creation;
	}
	filterPreload(preload) {
		this.preloads = [];
		if (preload.length) {
			for (var jsFile of this.jsFiles) {
				jsFile.preload = false;
			}
			for (var pattern of preload) {
				pattern = this.resolveSlash(pattern);
				var star = pattern.lastIndexOf("*");
				if (star >= 0) {
					pattern = pattern.slice(0, star);
					for (var result of this.jsFiles) {
						var target = result.target.slice(0, star);
						if (target == pattern) {
							result.preload = true;
							this.preloads.push(result.target);
						}
					}
				}
				else {
					pattern += ".xsb";
					for (var result of this.jsFiles) {
						var target = result.target;
						if (target == pattern) {
							result.preload = true;
							this.preloads.push(result.target);
						}
					}
				}
			}
		}
	}
	filterRecipe(name, pattern) {
		var star = pattern.lastIndexOf("*");
		if (star >= 0) {
			pattern = pattern.slice(0, star);
			for (var cFile of this.cFiles) {
				var target = cFile.target.slice(0, star);
				if (target == pattern)
					cFile.recipe = name;
			}
		}
		else {
			pattern += ".c";
			for (var cFile of this.cFiles) {
				var target = cFile.target;
				if (target == pattern)
					cFile.recipe = name;
			}
		}
	}
	filterRecipes(recipes) {
		this.recipes = {};
		for (var cFile of this.cFiles)
			cFile.recipe = null;
		for (var name in recipes) {
			var path = this.moddablePath + this.slash + "tools" + this.slash + "mcconfig" + this.slash;
			if (this.windows)
				path += "nmake.";
			else
				path += "make.";
			path += this.platform + "." + name + ".mk";
			this.recipes[name] = FS.readFileSync(path);
			var recipe = recipes[name];
			if (recipe instanceof Array) {
				for (var pattern of recipe)
					this.filterRecipe(name, pattern);
			}
			else if (recipe)
				this.filterRecipe(name, recipe);
		}
	}
	run() {
		super.run();
		
		this.filterCommonjs(this.manifest.commonjs);
		this.filterConfig(this.manifest.config);
		this.filterCreation(this.manifest.creation);
		this.defines = this.manifest.defines;
		this.filterPreload(this.manifest.preload);
		this.filterRecipes(this.manifest.recipes);
		this.strip = this.manifest.strip;
		
		var name = this.environment.NAME
		if ((this.platform == "x-ios") || (this.platform == "x-ios-simulator") || (this.platform == "x-mac"))
			this.binPath = this.createDirectories(this.outputPath, "bin", name + ".app");
		else if ((this.platform == "x-lin") || (this.platform == "x-win"))
			this.binPath = this.createDirectories(this.outputPath, "bin");
		else
			this.binPath = this.createDirectories(this.outputPath, "bin", name);
		this.tmpPath = this.createDirectories(this.outputPath, "tmp", name);
			
		this.modulesPath = this.tmpPath + this.slash + "modules";
		FS.mkdirSync(this.modulesPath);
		for (var folder of this.jsFolders)
			FS.mkdirSync(this.modulesPath + this.slash + folder);
		
		if ((this.platform == "x-android") || (this.platform == "x-android-simulator")) {
			var mainPath = this.environment.PROJECT, path, file;
			if (!mainPath) {
				mainPath = this.binPath;
				this.environment.PROJECT = mainPath;
			}
			mainPath += "/app";
			FS.mkdirSync(mainPath);
			mainPath += "/src";
			FS.mkdirSync(mainPath);
			mainPath += "/main";
			FS.mkdirSync(mainPath);
			
			path = mainPath + "/assets";
			FS.mkdirSync(path);
			this.dataPath = this.resourcesPath = path;
			
			path = mainPath + "/cpp";
			FS.mkdirSync(path);
			file = new CMakeListsFile(path + "/CMakeLists.txt");
			file.generate(this);
			
			path = mainPath + "/java";
			FS.mkdirSync(path);
			this.javaPath = path;
		}
		else if ((this.platform == "x-ios") || (this.platform == "x-ios-simulator")) {
			this.dataPath = this.resourcesPath = this.binPath;
		}
		else if (this.platform == "x-lin") {
			if (!this.environment.NAMESPACE)
				this.environment.NAMESPACE = "moddable.tech"
			var signature = this.environment.NAME + "." + this.environment.NAMESPACE;
			signature = signature.split(".").reverse();
			this.environment.DASH_SIGNATURE = signature.join("-");
			this.environment.DOT_SIGNATURE = signature.join(".");
			this.environment.SLASH_SIGNATURE = "/" + signature.join("/");
			this.dataPath = this.resourcesPath = this.tmpPath + this.slash + "resources";
			FS.mkdirSync(this.resourcesPath);
		}	
		else if (this.platform == "x-mac") {
			var path = this.binPath + "/Contents";
			FS.mkdirSync(path);
			this.binPath = path;
			FS.mkdirSync(path + "/MacOS");
			this.dataPath = this.resourcesPath = path + "/Resources";
			FS.mkdirSync(this.resourcesPath);
		}
		else {
			var folder = "mc", file;
			FS.mkdirSync(this.modulesPath + this.slash + folder);
			var source = this.tmpPath + this.slash + "mc.config.js";
			var target = folder + this.slash + "config.xsb";
			this.jsFiles.push({ source, target });
			file = new ConfigFile(source);
			file.generate(this);
			file = new DefinesFile(this.tmpPath + this.slash + "mc.defines.h");
			file.generate(this);
			file = new FormatFile(this.tmpPath + this.slash + "mc.format.h");
			file.generate(this);
			file = new RotationFile(this.tmpPath + this.slash + "mc.rotation.h");
			file.generate(this);
			this.dataPath = this.tmpPath + this.slash + "data";
			FS.mkdirSync(this.dataPath);
			this.resourcesPath = this.tmpPath + this.slash + "resources";
			FS.mkdirSync(this.resourcesPath);
		}
		for (var folder of this.dataFolders)
			FS.mkdirSync(this.dataPath + this.slash + folder);
		for (var folder of this.resourcesFolders)
			FS.mkdirSync(this.resourcesPath + this.slash + folder);

		var path = this.tmpPath + this.slash + "makefile", file;
		if (this.windows) {
			if (this.platform == "synergy")
				file = new SynergyNMakeFile(path);
			else if (this.platform == "esp")
				file = new espNMakeFile(path);
			else
				file = new NMakeFile(path);
		}
		else {
			if ((this.platform == "x-android") || (this.platform == "x-android-simulator"))
				file = new AndroidMakeFile(path);
			else
				file = new MakeFile(path);
		}
		file.generate(this);
		if (this.make) {
			if (this.windows)
				this.then("nmake", "/nologo", "/f", path);
			else
				this.then("make", "-f", path);
		}
	}
}
