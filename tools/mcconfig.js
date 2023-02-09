/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

import { MakeFile as MAKEFILE, TSConfigFile, PrerequisiteFile, FormatFile, RotationFile, Tool } from "mcmanifest";

var formatStrings = {
	gray16: "Gray16",
	gray256: "Gray256",
	rgb332: "RGB332",
	rgb565le: "RGB565LE",
	rgb565be: "RGB565BE",
	clut16: "CLUT16",
	argb4444: "ARGB4444",
	x: "",
};

var formatValues = {
	gray16: 4,
	gray256: 5,
	rgb332: 6,
	rgb565le: 7,
	rgb565be: 8,
	clut16: 11,
	argb4444: 12,
	x: 0,
};

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
				creation.keys.initial, ",", 
				creation.keys.incremental, ",", 
				creation.keys.name, ",", 
				creation.keys.symbol, ",",
				creation.parser.buffer, ",",
				creation.parser.table, ",",
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
		for (var result of [].concat(tool.nodered2mcuFiles, tool.cdvFiles)) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target + ".xsb");
		}
		for (var result of [].concat(tool.jsFiles, tool.tsFiles)) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}	
		for (var result of tool.cFiles) {
			var sourceParts = tool.splitPath(result.source);
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write(sourceParts.name);
			this.write(sourceParts.extension);
			this.write(".xsi");
		}
		for (var result of tool.hFiles) {
			var sourceParts = tool.splitPath(result);
			this.write("\\\n\t$(TMP_DIR)");
			this.write(tool.slash);
			this.write(sourceParts.name);
			this.write(".h.xsi");
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
			this.line("$(TMP_DIR)", tool.slash, sourceParts.name, sourceParts.extension, ".xsi: ", source);
			this.echo(tool, "xsid ", sourceParts.name, sourceParts.extension, ".xsi");
			this.line("\t$(XSID) ", source, " -o $(@D)");
		}
		for (var result of tool.hFiles) {
			var sourceParts = tool.splitPath(result);
			this.line("$(TMP_DIR)", tool.slash, sourceParts.name, ".h.xsi: ", result);
			this.echo(tool, "xsid ", sourceParts.name, ".h.xsi");
			this.line("\t$(XSID) ", result, " -o $(@D)");
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
			var buffer = tool.readFileString(source);
			var a = buffer.match(/package ([^;]+);/m);
			if (a.length != 2) {
				throw new Error("no package in ", source);
			}
			a = a[1].split(".");
			var path = tool.javaPath;
			for (var name of a) {
				path += "/" + name;
				tool.createDirectory(path);
			}			
			var parts = tool.splitPath(source);
			var target = a.join("/") + "/" + parts.name + ".java";
			this.write("\\\n\t$(SOURCES_DIR)/");
			this.write(target);
			result.target = target;
		}
		for (var result of tool.androidFiles) {
			this.write("\\\n\t");
			this.write(result.target);
		}
		this.line("");
		var task = tool.config.task;
		if (!task)
			task = "help"
		this.line("TASK = ", task);
	}
	generateObjectsRules(tool) {
		for (var result of tool.javaFiles) {
			this.line("$(SOURCES_DIR)/", result.target, ": ", result.source);
			this.echo(tool, "copy ", result.target);
			this.line("\tcp $< $@");
		}
		for (var result of tool.androidFiles) {
			this.line(result.target, ": ", result.source);
			this.echo(tool, "copy ", result.target);
			this.line("\tcp $< $@");
		}
	}
}

class IOSMakeFile extends MakeFile {
	constructor(path) {
		super(path)
	}
	generateObjectsDefinitions(tool) {
		var task = tool.config.task;
		if (!task)
			task = "xcode"
		this.line("TASK = ", task);
	}
	generateObjectsRules(tool) {
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

class esp32NMakeFile extends NMakeFile {
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
	generate(tool) {
		this.line('# WARNING: This file is automatically generated. Do not edit.');
		this.line("");
		this.line("cmake_minimum_required(VERSION 3.4.1)");
		this.line("");
		this.line("add_library(tech-moddable-piu SHARED");
		this.line("\t", tool.moddablePath, "/../moddableprojects/xs/platforms/android_xs.c");
		var names = tool.enumerateDirectory(tool.xsPath + "/sources");
		var c = names.length;
		for (var i = 0; i < c; i++) {
			var name = names[i];
			if (name.endsWith(".c") && (name != "xsDefaults.c") && (name != "xspcre.c"))
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
		if (tool.debug) {
			this.line("\tmxDebug=1");
			this.line("\tmxInstrument=1");
		}
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
		this.line("\t", tool.moddablePath, "/../moddableprojects/xs/platforms");
		this.line("\t", tool.xsPath, "/sources");
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
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		if (("screen" in tool.config) && tool.config.screen)
			this.line("import Screen from \"", tool.config.screen, "\";");
		if (("touch" in tool.config) && tool.config.touch)
			this.line("import Touch from \"", tool.config.touch, "\";");
		this.line("export default Object.freeze({");
		for (let key in tool.config) {
			if (key == "screen") {
				if (tool.config.screen)
					this.line("\tScreen,");
			}
			else if (key == "touch") {
				if (tool.config.touch)
					this.line("\tTouch,");
			}
			else {
				this.write("\t\"");
				this.write(key);
				this.write("\": ");
				this.write(JSON.stringify(tool.config[key], null, "\t"));
				this.line(",");
			}
		}
		this.line("}, true);");
		this.close();
	}
}

class DefinesFile extends PrerequisiteFile {
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		this.generateDefines(tool, "MODDEF_", tool.defines)
		this.line('#define PIU_DASH_SIGNATURE "', tool.environment.DASH_SIGNATURE, '"');
		this.line('#define PIU_DOT_SIGNATURE "', tool.environment.DOT_SIGNATURE, '"');
		this.line('#define PIU_SLASH_SIGNATURE "', tool.environment.SLASH_SIGNATURE, '"');
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
						if (value.every(item => "number" == typeof item)) {
							this.write(`#define ${prefix + name} {`);
							for (let i = 0, length = value.length; i < length; i++) {
								if (i)
									this.write(", ");
								this.write(value[i]);
							}
							this.write("}\n");
						}
						else
						if (value.every(item => "string" == typeof item)) {
							this.write(`#define ${prefix + name} \\\n`);
							for (let i = 0, length = value.length; i < length; i++) {
								if (i)
									this.write("\\\n");
								this.write(value[i]);
							}
							this.write("\n");
						}
					}
					else
						this.generateDefines(tool, prefix + name + "_", value);
					break;
			}
		}
	}
}

class XcodeFile extends PrerequisiteFile {
	addSource(path) {
		let parts = this.tool.splitPath(path);
		parts.fileReferenceID = this.id++;
		parts.buildFileID = this.id++;
		if (parts.extension == ".m")
			parts.fileType = "sourcecode.c.objc";
		else
			parts.fileType = "sourcecode.c.c";
		this.sources.push(parts);
	}
	addResource(path, folder) {
		let parts = this.tool.splitPath(path);
		parts.fileReferenceID = this.id++;
		parts.buildFileID = this.id++;
		if (parts.extension == ".framework") {
			if (folder && folder.indexOf("embed")>=0)
				parts.copyFileID = this.id++;
			parts.fileType = "wrapper.framework";
		}
		else if (parts.extension == ".bundle") {
			parts.copyFileID = this.id++;
			parts.fileType = "wrapper.plug-in";
		}
		else if (parts.extension == ".plist")
			parts.fileType = "text.plist.xml";
		else if (parts.extension == ".png")
			parts.fileType = "image.png";
		else if (parts.extension == ".jpg")
			parts.fileType = "image.jpg";
		else if (parts.extension == ".xcassets")
			parts.fileType = "folder.assetcatalog";
		else if (folder)
			parts.fileType = "folder";
		else
			parts.fileType = "file";
		this.resources.push(parts);
	}
	generate(tool) {
		this.resources = [];
		this.sources = [];
		
		this.id = 1;
		this.addSource(tool.moddablePath + "/../moddableprojects/xs/platforms/ios_xs.c");
		var names = tool.enumerateDirectory(tool.xsPath + "/sources");
		var c = names.length;
		for (var i = 0; i < c; i++) {
			var name = names[i];
			if (name.endsWith(".c") && (name != "xsDefaults.c") && (name != "xspcre.c"))
				this.addSource(tool.xsPath + "/sources/" + name);
		}
		for (var result of tool.cFiles) {
			this.addSource(result.source);
		}
		this.addSource(tool.tmpPath + "/mc.xs.c");
	
		for (var result of tool.dataFiles) {
			let target = result.target;
			if (target.indexOf("/") < 0)	
				this.addResource(tool.dataPath + "/" + target);
		}
		for (var result of tool.dataFolders) {
			this.addResource(tool.dataPath + "/" + result, true);
		}
		for (var result of tool.resourcesFiles) {
			let target = result.target;
			if (target.indexOf("/") < 0)	
				this.addResource(tool.resourcesPath + "/" + target, result.source);
		}
		for (var result of tool.resourcesFolders) {
			this.addResource(tool.resourcesPath + "/" + result, true);
		}
		for (var result of tool.stringFiles) {
			let target = result.target;
			this.addResource(tool.resourcesPath + "/" + target);
		}
		if (tool.stringFiles.length)
			this.addResource(tool.resourcesPath + "/" + tool.localsName + ".mhi");

		this.addResource(tool.mainPath + "/ios/Assets.xcassets", true);
	
		var path = tool.moddablePath + "/../moddableprojects/modules/piu/PC/ios/support/Xcode.txt";
		var template = tool.readFileString(path);
		
		template = template.replace(/#DEVELOPMENT_TEAM#/g, tool.environment.DEVELOPMENT_TEAM);
		template = template.replace(/#INFOPLIST_FILE#/g, tool.mainPath + "/ios/info.plist");
		template = template.replace(/#NAME#/g, tool.environment.NAME);
		template = template.replace(/#ORGANIZATIONNAME#/g, tool.environment.ORGANIZATION);
		template = template.replace(/#PRODUCT_BUNDLE_IDENTIFIER#/g, tool.environment.DOT_SIGNATURE);
		template = template.replace(/#IPHONEOS_DEPLOYMENT_TARGET#/g, tool.environment.IPHONEOS_DEPLOYMENT_TARGET || "8.0");
		template = template.replace(/#PUSH_ENABLED#/g, tool.environment.PUSH_ENABLED || "0");
		if (tool.environment.PUSH_ENABLED == "1" && tool.environment.CODE_SIGN_ENTITLEMENTS) {
			template = template.replace(/#CODE_SIGN_ENTITLEMENTS#/g, "CODE_SIGN_ENTITLEMENTS = " + tool.mainPath + "/ios/" + tool.environment.CODE_SIGN_ENTITLEMENTS + ";");
		}
		else {
			template = template.replace(/#CODE_SIGN_ENTITLEMENTS#/g, "");
		}
		this.current = ""
		this.line("\t\t\t\t\t\"INCLUDE_XSPLATFORM=1\",");
		this.line("\t\t\t\t\t\"XSPLATFORM=\\\\\\\"ios_xs.h\\\\\\\"\",");
		if (tool.debug) {
			this.line("\t\t\t\t\t\"mxDebug=1\",");
			this.line("\t\t\t\t\t\"mxInstrument=1\",");
		}
		this.line("\t\t\t\t\t\"mxRun=1\",");
		this.line("\t\t\t\t\t\"mxParse=1\",");
		this.line("\t\t\t\t\t\"mxNoFunctionLength=1\",");
		this.line("\t\t\t\t\t\"mxNoFunctionName=1\",");
		this.line("\t\t\t\t\t\"mxHostFunctionPrimitive=1\",");
		this.line("\t\t\t\t\t\"mxFewGlobalsTable=1\",");
		template = template.replace(/#GCC_PREPROCESSOR_DEFINITIONS#/g, this.current);
		
		this.current = ""
		this.line("\t\t\t\t\t", tool.xsPath, "/includes,");
		this.line("\t\t\t\t\t", tool.xsPath, "/platforms,");
		this.line("\t\t\t\t\t", tool.moddablePath, "/../moddableprojects/xs/platforms,");
		this.line("\t\t\t\t\t", tool.xsPath, "/sources,");
		for (var folder of tool.cFolders) {
			this.line("\t\t\t\t\t", folder, ",");
		}	
		this.line("\t\t\t\t\t", tool.tmpPath);
		template = template.replace(/#HEADER_SEARCH_PATHS#/g, this.current);
		
		template = template.replace(/#FRAMEWORK_SEARCH_PATHS#/g, tool.resourcesPath);

		this.current = ""
		this.sources.forEach(file => this.generatePBXBuildFile(file));
		this.resources.forEach(file => this.generatePBXBuildFile(file));
		template = template.replace(/#PBXBuildFile#/g, this.current);
		
		this.current = ""
		this.sources.forEach(file => this.generatePBXFileReference(file));
		this.resources.forEach(file => this.generatePBXFileReference(file));
		template = template.replace(/#PBXFileReference#/g, this.current);
		
		this.current = ""
		this.resources.forEach(file => this.generatePBXCopyFilesBuildPhase(file));
		template = template.replace(/#PBXCopyFilesBuildPhase#/g, this.current);
		
		this.current = ""
		this.resources.forEach(file => this.generatePBXFrameworksBuildPhase(file));
		template = template.replace(/#PBXFrameworksBuildPhase#/g, this.current);
		
		this.current = ""
		this.resources.forEach(file => this.generatePBXGroup(file));
		template = template.replace(/#PBXResourcesGroup#/g, this.current);
		
		this.current = ""
		this.sources.forEach(file => this.generatePBXGroup(file));
		template = template.replace(/#PBXSourcesGroup#/g, this.current);

		this.current = ""
		this.resources.forEach(file => this.generatePBXResourcesBuildPhase(file));
		template = template.replace(/#PBXResourcesBuildPhase#/g, this.current);

		this.current = ""
		this.sources.forEach(file => this.generatePBXBuildPhase(file));
		template = template.replace(/#PBXSourcesBuildPhase#/g, this.current);
		
		this.current = template;
		this.close();
	}
	generatePBXBuildFile(file) {
		this.write("\t\t");
		this.writeID(file.buildFileID);
		this.write(" /* ");
		this.write(file.name);
		this.write(file.extension);
		this.write(" in Sources */ = {isa = PBXBuildFile; fileRef = ");
		this.writeID(file.fileReferenceID);
		this.write(" /* ");
		this.write(file.name);
		this.write(file.extension);
		this.line(" */; };");
		if (file.fileType == "wrapper.framework" && file.copyFileID) {
			this.write("\t\t");
			this.writeID(file.copyFileID);
			this.write(" /* ");
			this.write(file.name);
			this.write(file.extension);
			this.write(" in Sources */ = {isa = PBXBuildFile; fileRef = ");
			this.writeID(file.fileReferenceID);
			this.write(" /* ");
			this.write(file.name);
			this.write(file.extension);
			this.line(" */; settings = {ATTRIBUTES = (CodeSignOnCopy, RemoveHeadersOnCopy, ); }; };");
		}
	}
	generatePBXCopyFilesBuildPhase(file) {
		if (file.fileType == "wrapper.framework" && file.copyFileID) {
			this.write("\t\t\t\t");
			this.writeID(file.copyFileID);
			this.write(" /* ");
			this.write(file.name);
			this.write(file.extension);
			this.line(" in Embed Frameworks */,");
		}
	}
	generatePBXFrameworksBuildPhase(file) {
		if (file.fileType == "wrapper.framework") {
			this.write("\t\t\t\t");
			this.writeID(file.buildFileID);
			this.write(" /* ");
			this.write(file.name);
			this.write(file.extension);
			this.line(" in Frameworks */,");
		}
	}
	generatePBXResourcesBuildPhase(file) {
		if (file.fileType != "wrapper.framework") {
			this.write("\t\t\t\t");
			this.writeID(file.buildFileID);
			this.write(" /* ");
			this.write(file.name);
			this.write(file.extension);
			this.line(" in Resources */,");
		}
	}
	generatePBXBuildPhase(file) {
		this.write("\t\t\t\t");
		this.writeID(file.buildFileID);
		this.write(" /* ");
		this.write(file.name);
		this.write(file.extension);
		this.line(" in Sources */,");
	}
	generatePBXFileReference(file) {
		this.write("\t\t");
		this.writeID(file.fileReferenceID);
		this.write(" /* ");
		this.write(file.name);
		this.write(file.extension);
		this.write(" */ = {isa = PBXFileReference; lastKnownFileType = ");
		this.write(file.fileType);
		this.write("; name = ");
		this.write(file.name);
		this.write(file.extension);
		this.write("; path = ");
		this.write(file.directory);
		this.write("/");
		this.write(file.name);
		this.write(file.extension);
		this.line("; sourceTree = \"<group>\"; };");
	}
	generatePBXGroup(file) {
		this.write("\t\t\t\t");
		this.writeID(file.fileReferenceID);
		this.write(" /* ");
		this.write(file.name);
		this.write(file.extension);
		this.line(" */,");
	}
	writeID(id) {
		id = id.toString();
		this.write("000000000000000000000000".slice(0, -id.length) + id);
	}
}

export default class extends Tool {
	constructor(argv) {
		super(argv);
		var prefix;
		var path = this.moddablePath + this.slash + "tools" + this.slash + "mcconfig" + this.slash;
		if (this.windows)
			prefix = "nmake.";
		else
			prefix = "make.";

		path += prefix + this.platform + ".mk";
			
		path = this.resolveFilePath(path);
		if (!path) {
//			throw new Error("unknown platform!");
		}
		else
			this.fragmentPath = path;
		this.nativeCode = true;
	}
	createDirectories(path, first, last) {
		this.createDirectory(path);
		path += this.slash + first;
		this.createDirectory(path);
		var platform = this.platform;
		if (platform.startsWith("x-cli-")) {
			path += this.slash + platform.slice(6);
			this.createDirectory(path);
		}
		else if (platform.startsWith("x-")) {
			path += this.slash + platform.slice(2);
			this.createDirectory(path);
		}
		else {
			path += this.slash + this.platform;
			this.createDirectory(path);
			if (this.subplatform) {
				path += this.slash + this.subplatform;
				this.createDirectory(path);
			}
			else if ((platform == "lin") || (platform == "mac") || (platform == "win")) {
				path += this.slash + "mc";
				this.createDirectory(path);
			}
		}
		if (this.debug) 
			path += this.slash + "debug";
		else if (this.instrument) 
			path += this.slash + "instrument";
		else
			path += this.slash + "release";
		this.createDirectory(path);
		if (last) {
			path += this.slash + last;
			this.createDirectory(path);
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
			if ("UNDEFINED" === this.format) {
				if (undefined !== config.format) {
					for (let property in formatStrings) {
						if (formatStrings[property] == config.format) {
							this.format = property;
							break;
						}
					}
					if ("UNDEFINED" === this.format)
						throw new Error(`invalid format: ${config.format}`);
				}
				else
					this.format = "rgb565le";
			}
			config.format = formatStrings[this.format];

			if (undefined === this.rotation) {
				if (undefined !== config.rotation) {
					if ((0 !== config.rotation) && (90 !== config.rotation) && (180 !== config.rotation) && (270 !== config.rotation))
						throw new Error(`invalid rotation: ${config.rotation}`);
					this.rotation = config.rotation;
				}
				else
					config.rotation = this.rotation = 0;
			}
			else
				config.rotation = this.rotation;
		}
		this.config = config;
	}
	filterCreation(creation) {
		creation.chunk ??= {};
		if (!creation.chunk.initial) creation.chunk.initial = 32768;
		creation.chunk.incremental ??= 1024;
		creation.heap ??= {};
		if (!creation.heap.initial) creation.heap.initial = 2048;
		creation.heap.incremental ??= 64;
		creation.stack ??= 384;
		creation.keys ??= {};
		if (creation.keys.initial) {
			creation.keys.incremental ??= 0;
		}
		else if (creation.keys.available) {
			creation.keys.initial = creation.keys.available;
			creation.keys.incremental = 0;
		}
		else {
			creation.keys.initial = 256;
			creation.keys.incremental = 0;
		}
		if (!creation.keys.name) creation.keys.name = 127;
		if (!creation.keys.symbol) creation.keys.symbol = 127;
		creation.parser ??= {};
		if (!creation.parser.buffer) creation.parser.buffer = 32768;
		if (!creation.parser.table) creation.parser.table = 1993;
		creation.static ??= 0;
		if (!creation.main) creation.main = "main";
		if ((this.platform == "x-android") || (this.platform == "x-android-simulator") || (this.platform == "x-ios") || (this.platform == "x-ios-simulator")) {
			creation.main = this.ipAddress;
		}
		this.creation = creation;
	}
	filterPreload(preload) {
		this.preloads = [];
		if (preload.length) {
			for (var file of [].concat(this.jsFiles, this.tsFiles, this.cdvFiles, this.nodered2mcuFiles)) {
				file.preload = false;
			}
			for (var pattern of preload) {
				pattern = this.resolvePrefix(pattern);
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
					for (var result of [].concat(this.jsFiles, this.tsFiles)) {
						var target = result.target;
						if (target == pattern) {
							result.preload = true;
							this.preloads.push(result.target);
						}
					}
					for (var result of [].concat(this.cdvFiles, this.nodered2mcuFiles)) {
						const target = result.target + ".xsb";
						if (target == pattern) {
							result.preload = true;
							this.preloads.push(target);
						}
					}
				}
			}
		}
	}
	filterRecipe(name, pattern) {
		var star = pattern.lastIndexOf("*");
		if (star == 0 && pattern.length > 2 && '.' == pattern[1]) {
			pattern = pattern.slice(1);
			for (var cFile of this.cFiles) {
				var target = cFile.target;
				if (target.endsWith(pattern))
					cFile.recipe = name;
			}
		}
		else if (star > 0) {
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
			this.recipes[name] = this.readFileString(path);
			var recipe = recipes[name];
			if (recipe instanceof Array) {
				for (var pattern of recipe)
					this.filterRecipe(name, pattern);
			}
			else if (recipe)
				this.filterRecipe(name, recipe);
		}
	}
	generateAndroidDirectory(from, to) {
		this.createDirectory(to);
		var names = this.enumerateDirectory(from);
		var c = names.length;
		for (var i = 0; i < c; i++) {
			var name = names[i];
			if (name[0] != ".") {
				var source = from + this.slash + name;
				var target = to + this.slash + name;
				if (this.isDirectoryOrFile(source) < 0)
					this.generateAndroidDirectory(source, target);
				else
					this.androidFiles.push({ source, target });
			}
		}
	}
	generateIOSDirectory(from, to) {
		var names = this.enumerateDirectory(from);
		var c = names.length;
		for (var i = 0; i < c; i++) {
			var name = names[i];
			if (name[0] != ".") {
				var source = from + this.slash + name;
				var target = to ? to + this.slash + name : name;
				if (this.isDirectoryOrFile(source) < 0) {
					this.resourcesFolders.push(target);
					this.generateIOSDirectory(source, target);
				}
				else {
					this.resourcesFiles.push({ source, target });
				}
			}
		}
	}
	run() {
		this.localsName = "locals";
		super.run();
		
		this.filterCommonjs(this.manifest.commonjs);
		this.filterConfig(this.manifest.config);
		this.filterCreation(this.manifest.creation);
		this.defines = this.manifest.defines;
		this.filterPreload(this.manifest.preload);
		this.filterRecipes(this.manifest.recipes);
		this.strip = this.manifest.strip;
		this.typescript = this.manifest.typescript;
		
		var name = this.environment.NAME
		if (this.platform == "x-mac")
			this.binPath = this.createDirectories(this.outputPath, "bin", name + ".app");
		else if ((this.platform == "x-lin") || (this.platform == "x-win") || (this.platform.startsWith("x-cli-")))
			this.binPath = this.createDirectories(this.outputPath, "bin");
		else
			this.binPath = this.createDirectories(this.outputPath, "bin", name);
		this.tmpPath = this.createDirectories(this.outputPath, "tmp", name);
		this.libPath = this.createDirectories(this.outputPath, "tmp", "lib");
			
		this.modulesPath = this.tmpPath + this.slash + "modules";
		this.createDirectory(this.modulesPath);
		for (var folder of this.jsFolders)
			this.createFolder(this.modulesPath, folder);

		if (this.platform == "esp32") {
			if (undefined === this.environment.SDKCONFIGPATH) {
				if (undefined === this.environment.ESP32_SUBCLASS)
					this.environment.SDKCONFIGPATH = this.buildPath + this.slash + "devices" + this.slash + "esp32" + this.slash + "xsProj-esp32";
				else
					this.environment.SDKCONFIGPATH = this.buildPath + this.slash + "devices" + this.slash + "esp32" + this.slash + "xsProj-" + this.environment.ESP32_SUBCLASS;
			}
		}
		
		if ((this.platform == "x-android") || (this.platform == "x-android-simulator")) {
			var mainPath = this.binPath, path, file;
			this.androidFiles = [];
			mainPath += "/app";
			this.generateAndroidDirectory(this.mainPath + this.slash + "android", mainPath);
			mainPath += "/src";
			this.createDirectory(mainPath);
			mainPath += "/main";
			this.createDirectory(mainPath);
			
			path = mainPath + "/assets";
			this.createDirectory(path);
			this.dataPath = this.resourcesPath = path;
			
			path = mainPath + "/cpp";
			this.createDirectory(path);
			file = new CMakeListsFile(path + "/CMakeLists.txt", this);
			file.generate(this);
			
			path = mainPath + "/java";
			this.createDirectory(path);
			this.javaPath = path;
		}
		else if ((this.platform == "x-ios") || (this.platform == "x-ios-simulator")) {
			var path, file;
			this.dataPath = this.resourcesPath = this.tmpPath + this.slash + "resources";
			this.createDirectory(this.resourcesPath);
			path = this.binPath + "/" + name + ".xcodeproj";
			this.createDirectory(path);
			path += "/project.pbxproj"
			var file = new XcodeFile(path, this);
			file.generate(this);
		}
		else if (this.platform == "x-lin") {
			this.dataPath = this.resourcesPath = this.tmpPath + this.slash + "resources";
			this.createDirectory(this.resourcesPath);
		}	
		else if (this.platform == "x-mac") {
			var path = this.binPath + "/Contents";
			this.createDirectory(path);
			this.binPath = path;
			this.createDirectory(path + "/MacOS");
			this.dataPath = this.resourcesPath = path + "/Resources";
			this.createDirectory(this.resourcesPath);
		}
		else if (this.platform == "x-win") {
			this.dataPath = this.resourcesPath = this.tmpPath + this.slash + "resources";
			this.createDirectory(this.resourcesPath);
		}
		else if (this.platform.startsWith("x-cli-")) {
		}
		else {
			var folder = "mc", file;
			this.createDirectory(this.modulesPath + this.slash + folder);
			var source = this.tmpPath + this.slash + "mc.config.js";
			var target = folder + this.slash + "config.xsb";
			this.jsFiles.push({ source, target });
			if (this.preloads.length)
				this.preloads.push("mc" + this.slash + "config.xsb");
			file = new ConfigFile(source, this);
			file.generate(this);
			file = new DefinesFile(this.tmpPath + this.slash + "mc.defines.h", this);
			file.generate(this);
			file = new FormatFile(this.tmpPath + this.slash + "mc.format.h", this);
			file.generate(this);
			file = new RotationFile(this.tmpPath + this.slash + "mc.rotation.h", this);
			file.generate(this);
			this.dataPath = this.resourcesPath = this.tmpPath + this.slash + "resources";
			this.createDirectory(this.resourcesPath);
		}
		for (var folder of this.dataFolders)
			this.createFolder(this.dataPath, folder);
		for (var folder of this.resourcesFolders)
			this.createFolder(this.resourcesPath, folder);
			
		var file = new DefinesFile(this.tmpPath + this.slash + "mc.defines.h", this);
		file.generate(this);

		var path = this.tmpPath + this.slash + "makefile", file;
		if (this.windows) {
			if (this.platform == "synergy")
				file = new SynergyNMakeFile(path);
			else if (this.platform == "esp")
				file = new espNMakeFile(path);
			else if (this.platform == "esp32")
				file = new esp32NMakeFile(path);
			else
				file = new NMakeFile(path);
		}
		else {
			if ((this.platform == "x-android") || (this.platform == "x-android-simulator"))
				file = new AndroidMakeFile(path);
			else if ((this.platform == "x-ios") || (this.platform == "x-ios-simulator"))
				file = new IOSMakeFile(path);
			else
				file = new MakeFile(path);
		}
		file.generate(this);

		if (this.tsFiles.length) {
			file = new TSConfigFile(this.modulesPath + this.slash + "tsconfig.json");
			file.generate(this);
		}

		if (this.make) {
			let cmd;
			if (this.buildTarget) {
				if (this.windows)
					cmd = ["nmake", "/nologo", "/f", path, this.buildTarget];
				else 
					cmd = ["make", "-f", path, this.buildTarget];
			} else {
				if (this.windows)
					cmd = ["nmake", "/nologo", "/f", path];
				else
					cmd = ["make", "-f", path];
			}

			if ("esp32" === this.platform) {
				if (!this.getenv("IDF_PATH"))
					throw new Error ("$IDF_PATH not set. See set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/esp32.md");

				if (this.spawn(this.windows ? "where" : "which", "idf.py") !== 0) { // IDF installed but not sourced
					if (this.windows)
						cmd = ["cmd", "/C", `set IDF_EXPORT_QUIET=1 && pushd %IDF_PATH% && "%IDF_TOOLS_PATH%\\idf_cmd_init.bat" && popd && ${cmd.join(" ")}`];
					else
						cmd = ["bash", "-c", `export IDF_EXPORT_QUIET=1 && source $IDF_PATH/export.sh && ${cmd.join(" ")}`];
				}
			}

			this.then.apply(this, cmd);
		}
	}
}
