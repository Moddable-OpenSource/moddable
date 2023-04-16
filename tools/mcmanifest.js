/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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
import UncodeRanges from "unicode-ranges";
import URL from "url";

var formatNames = {
	gray16: "gray16",
	gray256: "gray256",
	rgb332: "rgb332",
	rgb565le: "rgb565le",
	rgb565be: "rgb565be",
	clut16: "clut16",
	argb4444: "argb4444",
	x: "x",
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

export class MakeFile extends FILE {
	constructor(path) {
		super(path)
	}
	echo(tool, ...strings) {
		if (tool.windows)
			this.write("\t@echo # ");
		else
			this.write("\t@echo \"# ");
		for (var string of strings)
			this.write(string);
		if (tool.windows)
			this.write("\n");
		else
			this.write("\"\n");
	}
	generate(tool) {
		this.generateDefinitions(tool)
		if (tool.environment)				// override default .mk file
			if (tool.environment.MAKE_FRAGMENT)
				tool.fragmentPath = tool.environment.MAKE_FRAGMENT;
		if (undefined === tool.fragmentPath)
			throw new Error("unknown platform: MAKE_FRAGMENT not found!");
		this.write(tool.readFileString(tool.fragmentPath));
		this.line("");
		this.generateRules(tool)
		this.close();
	}
	generateDataDefinitions(tool) {
		this.write("DATA =");
		for (var result of tool.dataFiles) {
			this.write("\\\n\t$(DATA_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		this.line("");
		this.line("");
	}
	generateDataRules(tool) {
		for (var result of tool.dataFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(DATA_DIR)", tool.slash, target, ": ", source);
			this.echo(tool, "copy ", target);
			if (tool.windows)
				this.line("\tcopy /Y $** $@");
			else
				this.line("\tcp $< $@");
		}
		this.line("");
	}
	setConfigOption(sdkconfig, option) {
		let name = option.name;
		let value = option.value;
		let index = sdkconfig.indexOf(name);
		let changed = false;
		if (-1 != index) {
			++index;
			if ("n" == value) {
				if ("y" == sdkconfig.charAt(index + name.length)) {
					sdkconfig = sdkconfig.replace(new RegExp(name + ".*"), name + "=");
					changed = true;
				}
			}
			else {
				if (value != sdkconfig.charAt(index + name.length)) {
					sdkconfig = sdkconfig.replace(new RegExp(name + ".*"), name + "=" + value);
					changed = true;
				}
			}
		}
		return { sdkconfig, changed };
	}
	generateConfigurationRules(tool) {
		if ("esp32" !== tool.platform)
			return;

		if (!tool.environment.SDKCONFIGPATH)
			return;		// mcrun

		const ESP32_SUBCLASS = tool.environment.ESP32_SUBCLASS ?? "esp32";
		const baseConfigDirectory = tool.buildPath + tool.slash + "devices" + tool.slash + "esp32" + tool.slash + "xsProj-" + ESP32_SUBCLASS;
		const outputConfigDirectory = tool.outputPath + tool.slash + "tmp" + tool.slash + "esp32" + tool.slash + (tool.subplatform ?? "") + tool.slash + (tool.debug ? "debug" : (tool.instrument ? "instrument" : "release")) + tool.slash + tool.environment.NAME + tool.slash + "xsProj-" + ESP32_SUBCLASS;
		tool.createDirectory(outputConfigDirectory);

		let PARTITIONS_FILE = tool.environment.PARTITIONS_FILE;
		if (!PARTITIONS_FILE) {
			const PROJ_DIR_TEMPLATE = `${tool.buildPath}/devices/esp32/xsProj-${tool.environment.ESP32_SUBCLASS}`;
			PARTITIONS_FILE = `${PROJ_DIR_TEMPLATE}/partitions.csv`
		}
		let partitions = tool.readFileString(PARTITIONS_FILE);

		const usesMods = tool.defines.xs?.mods || tool.defines.XS_MODS;
		const wantsOTA = tool.defines.ota?.autosplit;
		const wantsStorage = tool.defines.file?.partition;

		if (wantsOTA || usesMods || wantsStorage) {
			let factoryLine, hasOTA, hasMod, hasStorage, storagePartition = wantsStorage?.slice(1);

			function parse(value) {
				if (value.startsWith("0x"))
					return parseInt(value, 16);
				if (value.endsWith("K"))
					return parseInt(value) * 1024;
				if (value.endsWith("M"))
					return parseInt(value) * 1024 * 1024;
				return parseInt(value);
			}

			partitions = partitions.split("\n");
			for (let i = 0; i < partitions.length; i++) {
				let line = partitions[i].trim();
				if (!line || line.startsWith("#")) continue;
				
				line = line.split(",").map(item => item.trim());
				if ("app" === line[1]) {
					const kind = line[2].toLowerCase();
					if (kind.startsWith("ota"))
						hasOTA = true;
					else if ("factory" === kind)
						factoryLine = i;
				}
				else if (("xs" === line[0]) && (("data" === line[1]) || (64 === parse(line[1]))))
					hasMod = true;
				else if ((storagePartition === line[0]) && ("data" === line[1]))
					hasStorage = true;
			}

			if (undefined !== factoryLine) {
				const OTADATA_SIZE = 0x2000;
				const MODS_SIZE = 0x40000;
				const STORAGE_SIZE = 0x10000;
				let line = partitions[factoryLine].split(",").map(item => item.trim());
				let size = parse(line[4]);

				if (!hasOTA && wantsOTA)
					size -= OTADATA_SIZE;	// space for ota_data
				if (!hasMod && usesMods)
					size -= MODS_SIZE;		// space for mods
				if (!hasStorage && wantsStorage)
					size -= STORAGE_SIZE;	// space for files

				if (!hasOTA && wantsOTA) {
					const size1 = Math.idiv(size >> 1, 0x10000) * 0x10000;		// "Partitions of type app have to be placed at offsets aligned to 0x10000"
					const size2 = size - size1;
		
					partitions[factoryLine] =
`ota_0, app, ota_0, ${line[3]}, 0x${size1.toString(16).toUpperCase()},
ota_1, app, ota_1, , 0x${size2.toString(16).toUpperCase()},
otadata, data, ota, , ${OTADATA_SIZE},`;
					tool.report(`mcconfig: ESP32 factory app partition divided into OTA partitions of size 0x${size1.toString(16)} and 0x${size2.toString(16)}`)
				}
				else
					partitions[factoryLine] = `factory, app, factory, , 0x${size.toString(16)}`;
				if (!hasMod && usesMods) {
					partitions[factoryLine] += "\n" + `xs, 0x40, 1, , 0x${MODS_SIZE.toString(16)}`,
					tool.report(`mcconfig: mod partition of size 0x${MODS_SIZE.toString(16)} created from factory app partition`)
				}
				if (!hasStorage && wantsStorage) {
					partitions[factoryLine] += "\n" + `storage, data, spiffs, , 0x${STORAGE_SIZE.toString(16)},`,
					tool.report(`mcconfig: file storage partition of size 0x${STORAGE_SIZE.toString(16)} created from factory app partition`)
				}
			}
			partitions = partitions.join("\n");
		}
		let buildPartitionsFile = outputConfigDirectory + tool.slash + "partitions.csv";
		tool.setenv("PARTITIONS_FILE", buildPartitionsFile);
		this.line("PARTITIONS_FILE = ", buildPartitionsFile);
		
		if (1 === tool.isDirectoryOrFile(buildPartitionsFile)) {
			if (partitions === tool.readFileString(buildPartitionsFile))
				partitions = undefined;		// hasn't changed, so don't write
		}
		if (partitions)
			tool.writeFileString(buildPartitionsFile, partitions);
		
		// Read base debug build sdkconfig.defaults file
		let mergedConfig = [];
		let regex = /[\r\n]+/gm;
			
		let baseConfigFile = baseConfigDirectory + tool.slash + "sdkconfig.defaults";
		let baseConfig = tool.readFileString(baseConfigFile);
		let baseConfigLength = baseConfig.length;

		tool.setenv("CONFIGDIR", outputConfigDirectory);
		this.line("CONFIGDIR = ", outputConfigDirectory);

		// For release builds merge base sdkconfig.defaults.release file
		if (tool.debug === false) {
			let releaseBaseConfigFile = baseConfigFile + ".release";
			let entries = tool.readFileString(releaseBaseConfigFile);
			mergedConfig = entries.split(regex);

			// For instrumented release builds merge base "sdkconfig.inst" file
			if (tool.instrument === true) {
				let instConfigFile = baseConfigDirectory + tool.slash + "sdkconfig.inst";
				let instConfig = tool.readFileString(instConfigFile);
				mergedConfig = mergedConfig.concat(instConfig.split(regex));
			}
		}
		
		// Merge any application sdkconfig files
		if (tool.environment.SDKCONFIGPATH != baseConfigDirectory) {
			let appConfigFile = tool.environment.SDKCONFIGPATH + tool.slash + "sdkconfig.defaults";
			if (1 == tool.isDirectoryOrFile(appConfigFile)) {
				let entries = tool.readFileString(appConfigFile);
				mergedConfig = mergedConfig.concat(entries.split(regex));
			}
			
			if ((false === tool.debug) && (1 == tool.isDirectoryOrFile(appConfigFile + ".release"))) {
				let entries = tool.readFileString(appConfigFile + ".release");
				mergedConfig = mergedConfig.concat(entries.split(regex));
			}
				
			if (tool.debug === false && tool.instrument === true) {
				appConfigFile = tool.environment.SDKCONFIGPATH + tool.slash + "sdkconfig.inst";
				if (1 == tool.isDirectoryOrFile(appConfigFile)) {
					let entries = tool.readFileString(appConfigFile);
					mergedConfig = mergedConfig.concat(entries.split(regex));
				}
			}
		}
		
		let port = tool.getenv("UPLOAD_PORT");
		if (port) {
			if (port.charAt(0) != '"')
				port = `"${port}"`;
			mergedConfig.push("CONFIG_ESPTOOLPY_PORT=" + port);
		}

		// Merge differences
		let appended = false;
		mergedConfig.forEach(option => {
			if (option.length && ('#' != option.charAt(0))) {
				let parts = option.split('=');
				let name = parts[0];
				let value = parts[1];
				let start = baseConfig.indexOf(name + "=");
				if (-1 != start) {
					start += name.length + 1;
					let end = start;
					let c = baseConfig.charAt(end);
					while (c != '\n' && c != '\r') {
						if (++end >= baseConfigLength)
							break;
						c = baseConfig.charAt(end);
					}
					let original = baseConfig.slice(start, end);
					if (original != value) {
						if ("n" == value)
							value = "";
						baseConfig = baseConfig.replace(new RegExp(name + "=.*"), name + "=" + value);
					}
				}
				else {
					if (!appended) {
						baseConfig += "\n";
						appended = true;
					}
					baseConfig += name + "=" + value + "\n";
				}
			}
		});

		//BLE configuration, moved from bles2gatt.js
		let defines = tool.defines;
		if (defines && ("ble" in defines)) {
			let server, client, nimble;
			client = server = false;
			if ("server" in defines.ble && true == defines.ble.server)
				server = true;
			if ("client" in defines.ble && true == defines.ble.client)
				client = true;
			nimble = ("esp32" == tool.platform) && !(tool.getenv("ESP32_BLUEDROID") === "1");

			let options = [];
			if (client || server) {
				options.push({ name: "CONFIG_BT_ENABLED", value: "y" });
				if (nimble) {
					options.push({ name: "CONFIG_BT_NIMBLE_ENABLED", value: "y" });
					options.push({ name: "CONFIG_BT_BLUEDROID_ENABLED", value: "n" });
					options.push({ name: "CONFIG_BTDM_CTRL_MODE_BLE_ONLY", value: "y" });
					options.push({ name: "CONFIG_BT_NIMBLE_SM_LEGACY", value: "y" });
					options.push({ name: "CONFIG_BT_NIMBLE_SM_SC", value: "y" });
					options.push({ name: "CONFIG_BT_NIMBLE_ROLE_PERIPHERAL", value: (server ? "y" : "n") });
					options.push({ name: "CONFIG_BT_NIMBLE_ROLE_CENTRAL", value: (client ? "y" : "n") });
				}
				else {
					options.push({ name: "CONFIG_BT_BLUEDROID_ENABLED", value: "y" });
					options.push({ name: "CONFIG_BT_NIMBLE_ENABLED", value: "n" });
					options.push({ name: "CONFIG_BT_BLE_SMP_ENABLE", value: "y" });
					options.push({ name: "CONFIG_BT_GATTS_ENABLE", value: (server ? "y" : "n") });
					options.push({ name: "CONFIG_BT_GATTC_ENABLE", value: (client ? "y" : "n") });
				}
			} else {
				options.push({ name: "CONFIG_BT_ENABLED", value: "n" });
			}

			for (let i = 0; i < options.length; ++i) {
				let result = this.setConfigOption(baseConfig, options[i]);
				if (result.changed) {
					baseConfig = result.sdkconfig;
				}
			}
		}
		
		// Write the result, if it has changed
		let buildConfigFile = outputConfigDirectory + tool.slash + "sdkconfig.mc";
		tool.setenv("SDKCONFIG_FILE", buildConfigFile);
		this.line("SDKCONFIG_FILE=", buildConfigFile);
		if (tool.isDirectoryOrFile(buildConfigFile) == 1){
			const oldConfig = tool.readFileString(buildConfigFile);
			if (oldConfig == baseConfig) return;
		}
		tool.writeFileString(buildConfigFile, baseConfig);
	}
	generateBLEDefinitions(tool) {
		this.write("BLE =");
		this.write("\\\n\t$(TMP_DIR)");
		this.write(tool.slash);
		this.write("mc.bleservices");
		this.line("");
		this.line("");
	}
	generateBLERules(tool) {
		let defines = tool.defines;
		let client = false;
		let server = false;
		let nimble = false;
		if (defines && ("ble" in defines)) {
			if ("server" in defines.ble && true == defines.ble.server)
				server = true;
			if ("client" in defines.ble && true == defines.ble.client)
				client = true;
			nimble = ("esp32" == tool.platform) && !(tool.getenv("ESP32_BLUEDROID") === "1");
		}
		this.write("$(TMP_DIR)");
		this.write(tool.slash);
		this.write("mc.bleservices:");
		for (var result of tool.bleServicesFiles)
			this.write(` ${result.source}`);
		this.line("");
		if (tool.bleServicesFiles.length) {
			let platform = (nimble ? 'nimble' : tool.platform);
			this.echo(tool, "bles2gatt bleservices");
			if (server) {
				if (tool.windows)
					this.line(`\ttype nul >> ${tool.moddablePath}/modules/network/ble/${platform}/modBLEServer.c`);
				else
					this.line(`\ttouch ${tool.moddablePath}/modules/network/ble/${platform}/modBLEServer.c`);
			}
			if (client) {
				if (tool.windows)
					this.line(`\ttype nul >> ${tool.moddablePath}/modules/network/ble/${platform}/modBLEClient.c`);
				else
					this.line(`\ttouch ${tool.moddablePath}/modules/network/ble/${platform}/modBLEClient.c`);
			}
		}
		this.write("\t$(BLES2GATT)");
		if (tool.bleServicesFiles.length)
			this.write(tool.windows ? " $**" : " $^");
		if (client)
			this.write(" -c");
		if (server)
			this.write(" -v");
		if (nimble)
			this.write(" -n");
		if ("esp32" == tool.platform) {
			let sdkconfigFile = tool.getenv("SDKCONFIG_FILE");
			this.write(" -s ");
			this.write(sdkconfigFile);
			if (tool.windows) {
				let idfBuildDir = tool.outputPath + "\\tmp\\" + tool.environment.PLATFORMPATH + "\\" + (tool.debug ? "debug\\idf" : "release\\idf");
				let idfBuildDirMinGW = idfBuildDir.replace(/\\/g, "/");
				tool.setenv("IDF_BUILD_DIR_MINGW", idfBuildDirMinGW);
				if (sdkconfigFile  !== undefined){
					let sdkconfigFileMinGW = sdkconfigFile.replace(/\\/g, "/");
					tool.setenv("SDKCONFIG_FILE_MINGW", sdkconfigFileMinGW);
				}
				let binDirMinGW = tool.binPath.replace(/\\/g, "/");
				tool.setenv("BIN_DIR_MINGW", binDirMinGW);
			}
		}
		this.write(" -o $(TMP_DIR)");
		this.line("");
		this.line("");
	}
	generateDefinitions(tool) {
		this.line('# WARNING: This file is automatically generated. Do not edit. #');
		if (tool.debug)
			this.line("DEBUG = 1");
		if (tool.debug || tool.instrument)
			this.line("INSTRUMENT = 1");
		if (tool.verbose)
			this.line("VERBOSE = 1");
		for (var result in tool.environment)
			this.line(result, " = ", tool.environment[result].replace(/ /g, "\\ "));
		this.line("");

		this.line("BIN_DIR = ", tool.binPath);
		this.line("BUILD_DIR = ", tool.buildPath);
		this.line("DATA_DIR = ", tool.dataPath);
		this.line("MAIN_DIR = ", tool.mainPath);
		this.line("MODULES_DIR = ", tool.modulesPath);
		this.line("RESOURCES_DIR = ", tool.resourcesPath);
		this.line("TMP_DIR = ", tool.tmpPath);
		this.line("LIB_DIR = ", tool.libPath);
		this.line("XS_DIR = ", tool.xsPath);
		this.line("XSBUG_HOST = ", tool.xsbug?.host ?? "localhost");
		this.line("XSBUG_PORT = ", tool.xsbug?.port ?? 5002);
		if (tool.xsbugLog)
			this.line("XSBUG_LOG = 1");
		
		this.line("");

		this.generateManifestDefinitions(tool);
		this.generateModulesDefinitions(tool);
		this.generateObjectsDefinitions(tool);
		this.generateDataDefinitions(tool);
		this.generateBLEDefinitions(tool);
		this.generateResourcesDefinitions(tool);
	}
	generateManifestDefinitions(tool) {
		this.write("MANIFEST =");
		for (var result in tool.manifests.already) {
			this.write(" \\\n\t");
			this.write(result);
		}
		this.line("");
		this.line("");
	}
	generateModulesDefinitions(tool) {
		this.write("MODULES =");
		for (var result of tool.nodered2mcuFiles) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target + ".xsb");
		}
		for (var result of tool.cdvFiles) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target + ".xsb");
		}
		for (var result of tool.jsFiles) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.tsFiles) {
			this.write("\\\n\t$(MODULES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		this.line("");
		this.line("");
	}
	generateModulesRules(tool) {
		const generatedTS = [];

		for (let result of tool.nodered2mcuFiles) {
			let source = result.source;
			let target = result.target;
			const extension = ".js";
			const output = "$(MODULES_DIR)" + tool.slash + target + extension;
			this.line(output, ": ", source);
			this.echo(tool, "nodered2mcu ", target);
			this.line("\tnodered2mcu ", source, " -o $(@D)");

			tool.jsFiles.push({
				source: tool.modulesPath + tool.slash + target + extension,
				target: target + ".xsb"
			});
		}

		for (let result of tool.cdvFiles) {
			let source = result.source;
			let target = result.target;
			const extension = ("typescript" === result.query?.language) ? ".ts" : ".js";
			const output = "$(MODULES_DIR)" + tool.slash + target + extension;
			this.line(output, ": ", source);
			this.echo(tool, "cdv ", target);
			let pragmas = "";
			for (const name in result.query)
				pragmas += " " + "-p " + name + "=" + result.query[name];
			this.line("\tcdv ", source, " -o $(@D)", " -n ", target, pragmas);

			if (".js" === extension) {
				tool.jsFiles.push({
					source: tool.modulesPath + tool.slash + target + extension,
					target: target + ".xsb"
				});
			}
			else {
				tool.tsFiles.push({
					source: tool.modulesPath + tool.slash + target + extension,
					target: target + ".xsb"
				});
				generatedTS.push(output);
			}
		}

		for (var result of tool.jsFiles) {
			var source = result.source;
			var sourceParts = tool.splitPath(source);
			var target = result.target;
			var targetParts = tool.splitPath(target);
			this.line("$(MODULES_DIR)", tool.slash, target, ": ", source);
			this.echo(tool, "xsc ", target);
			var options = "";
			if (result.commonjs)
				options += " -p";
			if (tool.debug)
				options += " -d";
			if (tool.nativeCode)
				options += " -c";
			this.line("\t$(XSC) ", source, options, " -e -o $(@D) -r ", targetParts.name);
		}
		this.line("");
		
		if (tool.tsFiles.length) {
			let directories = tool.tsFiles.map(item => tool.splitPath(item.source).directory);
			const length = directories.length;
			let common;
			if (length > 1) {
				directories.sort();
				let first = directories[0].split(tool.slash);
				let last = directories[length - 1].split(tool.slash);
				const c = Math.min(first.length, last.length);
				let i = 0;
				while (i < c) {
					if (first[i] != last[i])
						break;
					i++;
				}
				common = first.slice(0, i).join(tool.slash).length;
			}
			else
				common = directories[0].length;
			var temporaries = [];
			for (var result of tool.tsFiles) {
				var source = result.source;
				var target = result.target;
				var targetParts = tool.splitPath(target);
				var temporary = source.slice(common, -3) + ".js"
				this.line("$(MODULES_DIR)", tool.slash, target, ": $(MODULES_DIR)", temporary);
				this.echo(tool, "xsc ", target);
				var options = "";
				if (result.commonjs)
					options += " -p";
				if (tool.debug)
					options += " -d";
				if (tool.nativeCode)
					options += " -c";
				this.line("\t$(XSC) $(MODULES_DIR)", temporary, options, " -e -o $(@D) -r ", targetParts.name);
				if (tool.windows)
					this.line("$(MODULES_DIR)", temporary, ": TSCONFIG");
				temporaries.push("%" + temporary);
			}
			if (tool.windows)
				this.line("TSCONFIG:");
			else
				this.line(temporaries.join(" "), " : ", "%", tool.slash, "tsconfig.json ", generatedTS.join(" "));
			this.echo(tool, "tsc ", "tsconfig.json");
			this.line("\t", tool.typescript.compiler, " -p $(MODULES_DIR)", tool.slash, "tsconfig.json");
			this.line("");
		}
	}
	generateObjectsDefinitions(tool) {
	}
	generateObjectsRules(tool) {
	}
	generateResourcesDefinitions(tool) {
		this.write("RESOURCES = $(STRINGS)");
		for (var result of tool.resourcesFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.bmpColorFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.bmpAlphaFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.bmpFontFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.bmpMaskFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		if (tool.format?.startsWith("clut")) {
			for (var result of tool.clutFiles) {
				this.write("\\\n\t$(RESOURCES_DIR)");
				this.write(tool.slash);
				this.write(result.target);
			}
		}
		for (var result of tool.imageFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.outlineFontFiles) {
			result.faces.forEach(face => {
				this.write("\\\n\t$(RESOURCES_DIR)");
				this.write(tool.slash);
				if ("-alpha" === face.suffix) {
					this.write(face.name + `-${face.size}.fnt`);
					this.write("\\\n\t$(RESOURCES_DIR)");
					this.write(tool.slash);
					this.write(face.name + `-${face.size}-alpha.bmp`);
				}
				else if ("-mask" === face.suffix) {
					this.write(face.name + `-${face.size}.bf4`);
				}
			});
		}
		for (var result of tool.soundFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		for (var result of tool.stringFiles) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(result.target);
		}
		if (tool.stringFiles.length) {
			this.write("\\\n\t$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(tool.localsName + ".mhi");
		}
		this.line("");
		this.line("");
	}
	generateResourcesRules(tool) {
		var formatPath = "$(TMP_DIR)" + tool.slash + "mc.format.h";
		var rotationPath = "$(TMP_DIR)" + tool.slash + "mc.rotation.h";

		for (var result of tool.resourcesFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", source);
			this.echo(tool, "copy ", target);
			if (tool.isDirectoryOrFile(source) < 0) {
				if (tool.windows)
					this.line("\tcopy /E /Y $** $@");
				else
					this.line("\tcp -R $< $@");
			}
			else {
				if (tool.windows)
					this.line("\tcopy /Y $** $@");
				else
					this.line("\tcp $< $@");
			}
		}

		if (tool.clutFiles) {
			if (!tool.format?.startsWith("clut")) {
				tool.clutFiles.length = 0;
				for (var result of tool.bmpColorFiles)
					delete result.clutName;
			}

			for (var result of tool.clutFiles) {
				var source = result.source;
				var target = result.target;
				this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", source);
				this.echo(tool, "buildclut ", target);
				this.line("\t$(BUILDCLUT) ", source, " -o $(@D)");
			}
		}

		for (var result of tool.bmpAlphaFiles) {
			var target = result.target;
			if (result.colorFile)
				this.line("$(RESOURCES_DIR)", tool.slash, target, ": $(RESOURCES_DIR)", tool.slash, result.colorFile.target);
			else {
				var parts = tool.splitPath(target);
				var source = result.source;
				var sources = result.sources;
				var manifest = "";
				var name = " -n " + parts.name.slice(0, -6);
				if (sources) {
					for (var path of sources)
						source += " " + path;
					manifest = "  $(MANIFEST)";
				}
				this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", source, " ", rotationPath, manifest);
				this.echo(tool, "png2bmp ", target);
				this.write("\t$(PNG2BMP) ");
				this.write(source);
				this.write(" -a");
				if (result.monochrome)
					this.write(" -m -4");
				this.write(" -o $(@D) -r ");
				this.write(tool.rotation);
				this.line(name);
			}
		}

		for (var result of tool.bmpColorFiles) {
			var target = result.target;
			var parts = tool.splitPath(target);
			var source = result.source;
			var alphaTarget = result.alphaFile ? result.alphaFile.target : null;
			var clutSource = result.clutName ? "$(RESOURCES_DIR)" + tool.slash + result.clutName + ".cct" : null;
			var sources = result.sources;
			var manifest = "";
			var name = " -n " + parts.name.slice(0, -6);
			if (sources) {
				for (var path of sources)
					source += " " + path;
				manifest = "  $(MANIFEST)";
			}

			this.write("$(RESOURCES_DIR)");
			this.write(tool.slash);
			this.write(target);
			this.write(": ");
			this.write(source);
			if (clutSource) {
				this.write(" ");
				this.write(clutSource);
			}
			this.write(" ");
			this.write(formatPath);
			this.write(" ");
			this.write(rotationPath);
			this.line(manifest);

			if (tool.windows)
				this.write("\t@echo # png2bmp ");
			else
				this.write("\t@echo \"# png2bmp ");
			this.write(target);
			if (alphaTarget) {
				this.write(" ");
				this.write(alphaTarget);
			}
			if (tool.windows)
				this.line("");
			else
				this.line("\"");
			this.write("\t$(PNG2BMP) ");
			this.write(source);
			if (!alphaTarget)
				this.write(" -c");
			if (result.monochrome)
				this.write(" -m -4");
			else {
				this.write(" -f ");
				if (result.format)
					this.write(result.format);
				else
					this.write(tool.format);
				if (clutSource) {
					this.write(" -clut ");
					this.write(clutSource);
				}
			}
			this.write(" -o $(@D) -r ");
			this.write(tool.rotation);
			this.line(name);
		}

		for (var result of tool.bmpFontFiles) {
			var parts;
			var source = result.source;
			parts = tool.splitPath(source);
			parts.extension = ".png";
			var pngSource = tool.joinPath(parts);
			var target = result.target;
			parts = tool.splitPath(target);
			var bmpTarget = parts.name + "-alpha.bmp";
			var bmpSource = "$(RESOURCES_DIR)" + tool.slash + bmpTarget;
			this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", source, " ", bmpSource, " ", rotationPath);
			this.echo(tool, "compressbmf ", target);
			this.line("\t$(COMPRESSBMF) ", source, " -i ", bmpSource, " -o $(@D) -r ", tool.rotation);
			this.line(bmpSource, ": ", pngSource, " ", rotationPath);
			this.echo(tool, "png2bmp ", bmpTarget);
			this.line("\t$(PNG2BMP) ", pngSource, " -a -o $(@D) -r ", tool.rotation, " -t");
		}

		for (var result of tool.bmpMaskFiles) {
			var target = result.target;
			var parts = tool.splitPath(target);
			var source = result.source;
			var bmpTarget = parts.name + ".bmp";
			var bmpSource = "$(RESOURCES_DIR)" + tool.slash + bmpTarget;
			this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", bmpSource);
			this.echo(tool, "rle4encode ", target);
			this.line("\t$(RLE4ENCODE) ", bmpSource, " -o $(@D)");
			var sources = result.sources;
			var manifest = "";
			var name = " -n " + parts.name.slice(0, -6);
			if (sources) {
				for (var path of sources)
					source += " " + path;
				manifest = "  $(MANIFEST)";
			}
			this.line(bmpSource, ": ", source, " ", rotationPath, manifest);
			this.echo(tool, "png2bmp ", bmpTarget);
			this.line("\t$(PNG2BMP) ", source, " -a -o $(@D) -r ", tool.rotation, " -t ", name);
		}

		for (var result of tool.imageFiles) {
			var source = result.source;
			var target = result.target;
			if (result.quality !== undefined) {
				var temporary = target + result.quality;
				this.line("$(RESOURCES_DIR)", tool.slash, temporary, ": ", source, " ", rotationPath);
				this.echo(tool, "image2cs ", temporary);
				this.line("\t$(IMAGE2CS) ", source, " -o $(@D) -q ", result.quality, " -r ", tool.rotation);
				this.line("$(RESOURCES_DIR)", tool.slash, target, ": $(RESOURCES_DIR)", tool.slash, temporary);
				this.echo(tool, "copy ", target);
				if (tool.windows)
					this.line("\tcopy /Y $** $@");
				else
					this.line("\tcp $< $@");
			}
			else {
				this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", source, " ", rotationPath);
				this.echo(tool, "image2cs ", target);
				this.line("\t$(IMAGE2CS) ", source, " -o $(@D) -r ", tool.rotation);
			}
		}

		for (var result of tool.outlineFontFiles) {
			var source = result.source;
			
			if (!tool.getenv("FONTBM"))
				throw new Error("$(FONTBM) environment variable not set. Is fontbm installed?");
			
			result.faces.forEach(face => {
				const name = face.name + "-" + face.size;

				let line = Array.of("$(RESOURCES_DIR)", tool.slash, ("-alpha" === face.suffix) ? `${name}.fnt` : `${name}.bf4`, ": ", source,
							" ", "$(RESOURCES_DIR)", tool.slash, name + ".txt",
							" ", "$(RESOURCES_DIR)", tool.slash, name + ".json");
				if (face.localization)
					line.push(" ", "$(RESOURCES_DIR)", tool.slash, "locals.mhi");
				this.line.apply(this, line);
				this.echo(tool, "fontbm ", name);

				let characters = face.characters ?? "";
				let blocks = ("string" === typeof face.blocks) ? [face.blocks] : (face.blocks ?? (characters ? [] : ["Basic Latin"]));
				blocks.forEach(block => {
					const info = UncodeRanges.find(info => info.category === block);
					if (!info)
						tool.reportWarning(NULL, 0, `Unknown Unicode block: "${block}"`);
					const count = info.range[1] - info.range[0] + 1;
					const c = new Array(count);
					for (let i = 0; i < count; i++)
						c[i] = String.fromCharCode(i + info.range[0]);
					characters += c.join("");
				});
				
				let path = tool.resourcesPath + tool.slash + name + ".txt";
				let former = tool.isDirectoryOrFile(path) ? tool.readFileString(path) : "";
				if (former !== characters)
					tool.writeFileBuffer(path, ArrayBuffer.fromString(characters));

				path = tool.resourcesPath + tool.slash + name + ".json";
				let options = JSON.stringify({kern: face.kern ?? false, monochrome: face.monochrome ?? false, localization: face.localization ?? false, source, rotation: tool.rotation});
				former = tool.isDirectoryOrFile(path) ? tool.readFileString(path) : "";
				if (former !== options)
					tool.writeFileString(path, options);				

				const localization = face.localization ? `--chars-file "$(RESOURCES_DIR)${tool.slash}locals.txt"` : "";
				this.line(`\t$(FONTBM) --font-file ${source} --font-size ${face.size} --output "$(RESOURCES_DIR)${tool.slash}${name}" --texture-crop-width --texture-crop-height --texture-name-suffix none --data-format bin ${face.kern ? "--kerning-pairs regular" : ""} ${face.monochrome ? "--monochrome" : ""} --chars-file "$(RESOURCES_DIR)${tool.slash}${name}.txt" ${localization}`);
				if ("-alpha" === face.suffix) {
					this.line("$(RESOURCES_DIR)", tool.slash, name + "-alpha.bmp", ": ", "$(RESOURCES_DIR)", tool.slash, `${name}.fnt`);
					this.line("\t$(PNG2BMP) ", "$(RESOURCES_DIR)", tool.slash, name + ".png", ` -a -o $(@D) ${face.monochrome ? "-m" : ""} -r `, tool.rotation, " -t");
				}
				else if ("-mask" === face.suffix) {
					this.line("\t$(PNG2BMP) ", "$(RESOURCES_DIR)", tool.slash, name + ".png", " -a -o $(@D) -r ", tool.rotation, " -t");
					this.line("\t$(COMPRESSBMF) ", "$(RESOURCES_DIR)", tool.slash, name + ".fnt", " -i ", "$(RESOURCES_DIR)", tool.slash, name + "-alpha.bmp", " -o $(@D) -r ", tool.rotation);
				}
			});
		}

		let bitsPerSample = 16, numChannels = 1, sampleRate = 11025, audioFormat = "uncompressed";
		let defines = tool.defines;
		if (defines) {
			let audioOut = defines.audioOut;
			if (audioOut) {
				if ("bitsPerSample" in audioOut) bitsPerSample = audioOut.bitsPerSample;
				if ("numChannels" in audioOut) numChannels = audioOut.numChannels;
				if ("sampleRate" in audioOut) sampleRate = audioOut.sampleRate;
				if ("format" in audioOut) audioFormat = audioOut.format;
			}
		}
		for (var result of tool.soundFiles) {
			var source = result.source;
			var target = result.target;
			this.line("$(RESOURCES_DIR)", tool.slash, target, ": ", source);
			this.echo(tool, "wav2maud ", target);
			this.line("\t$(WAV2MAUD) ", source, " -o $(@D) -r ", sampleRate, " -c ", numChannels, " -s ", bitsPerSample, " -f ", audioFormat);
		}

		for (var result of tool.stringFiles)
			this.line("$(RESOURCES_DIR)", tool.slash, result.target, ": ", "$(RESOURCES_DIR)", tool.slash, tool.localsName, ".mhi");
		this.write("$(RESOURCES_DIR)");
		this.write(tool.slash);
		this.write(tool.localsName + ".mhi: $(HEADERS)");
		for (var result of tool.stringFiles) {
			this.write(" ");
			this.write(result.source);
		}
		this.line("");
		this.echo(tool, "mclocal strings");
		this.write("\t$(MCLOCAL)");
		for (var result of tool.stringFiles) {
			this.write(" ");
			this.write(result.source);
		}
		if (!defines || !defines.locals || !defines.locals.all)
			this.write(" -d");
		if (tool.format)
			this.write(" -s");
		this.line(" -o $(@D) -r ", tool.localsName);
		this.line("");
	}
	generateRules(tool) {
		this.generateModulesRules(tool);
		this.generateObjectsRules(tool);
		this.generateDataRules(tool);
		this.generateConfigurationRules(tool);
		this.generateBLERules(tool);
		this.generateResourcesRules(tool);
	}
}

export class TSConfigFile extends FILE {
	constructor(path) {
		super(path);
	}
	generate(tool) {
		let json = {
			compilerOptions: {
				baseUrl: "./",
				forceConsistentCasingInFileNames: true,
				module: "es2020",
				outDir: tool.modulesPath,
				paths: {
				},
				lib: ["es2020"],
				sourceMap: true,
				target: "ES2020",
				...tool.typescript.tsconfig?.compilerOptions
			},
			files: [
			]
		}
		var paths = json.compilerOptions.paths;
		for (var result of tool.dtsFiles) {
			var specifier = result.target.slice(0, -2);
			if (tool.windows)
				specifier = specifier.replaceAll("\\", "/");
			specifier = tool.unresolvePrefix(specifier);
			paths[specifier] = [ result.source.slice(0, -5) ];
		}
		for (var result of tool.tsFiles) {
			var specifier = result.target.slice(0, -4);
			if (tool.windows)
				specifier = specifier.replaceAll("\\", "/");
			paths[specifier] = [ result.source.slice(0, -3) ];
			json.files.push(result.source);
		}
		this.write(JSON.stringify(json, null, "\t"));
		this.close();
	}
}

export class PrerequisiteFile {
	constructor(path, tool) {
		this.path = path;
		this.tool = tool;
		this.former = tool.isDirectoryOrFile(path) ? tool.readFileString(path) : "";
		this.current = ""
	}
	close() {
		if (this.former.localeCompare(this.current))
			this.tool.writeFileString(this.path, this.current);
	}
	line(...strings) {
		for (var string of strings)
			this.write(string);
		this.write("\n");
	}
	write(string) {
		this.current += string;
	}
}

export class FormatFile extends PrerequisiteFile {
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		this.line("#define kCommodettoBitmapFormat ", formatValues[tool.format]);
		this.close();
	}
}

export class RotationFile extends PrerequisiteFile {
	generate(tool) {
		this.line('/* WARNING: This file is automatically generated. Do not edit. */');
		this.line("");
		this.line("#define kPocoRotation ", tool.rotation);
		this.close();
	}
}

class Rule {
	constructor(tool) {
		this.tool = tool;
	}
	appendFile(files, target, source, include) {
		this.count++;
		source = this.tool.resolveFilePath(source);
		if (!files.already[source]) {
			files.already[source] = true;
			if (include) {
				let result = files.find(file => file.target == target);
				if (!result) {
					//this.tool.report(target + " " + source);
					let result = { target, source };
					files.push(result);
					return result;
				}
				if (result.sources)
					result.sources.push(source);
				else
					result.sources = [source];
			}
		}
	}
	appendFolder(folders, folder) {
		this.count++;
		if (!folders.already[folder]) {
			folders.already[folder] = true;
			folders.push(folder);
		}
	}
	appendSource(target, source, include, suffix, parts, kind) {
	}
	appendTarget(target) {
	}
	iterate(target, sourceIn, include, suffix, straight) {
		var tool = this.tool;
		var source = (typeof sourceIn == "string") ? sourceIn : sourceIn.source;
		var slash = source.lastIndexOf(tool.slash);
		var directory = this.tool.resolveDirectoryPath(source.slice(0, slash));
		if (directory) {
			this.count = 0;
			var star = source.lastIndexOf("*");
			var prefix = (star >= 0) ? source.slice(slash + 1, star) : source.slice(slash + 1);
			var names = tool.enumerateDirectory(directory);
			var c = names.length;
			for (var i = 0; i < c; i++) {
				var name = names[i];
				if (name[0] == ".")
					continue;
				var path = directory + tool.slash + name;
				var parts = tool.splitPath(path);
				if (star >= 0) {
					if (prefix) {
						if (parts.name.startsWith(prefix))
							name = parts.name.slice(prefix.length);
						else
							continue;
					}
					else
						name = parts.name;
				}
				else {
					if (parts.name == prefix)
						name = prefix;
					else
						continue;
				}
				var kind = tool.isDirectoryOrFile(path);
				var query = (typeof sourceIn === "string") ? {} : {...sourceIn};
				if (straight)
					this.appendSource(target, path, include, suffix, parts, kind, query);
				else
					this.appendSource(target + name, path, include, suffix, parts, kind, query);
			}
			if (!this.count)
				this.noFilesMatch(source, star);
		}
		else
			tool.reportError(null, 0, "directory not found: " + source);
	}
	noFilesMatch(target) {
	}
	process(property) {
		var tool = this.tool;
		var target = "~";
		if (target in property) {
			var sources = property[target];
			if (sources instanceof Array) {
				for (var source of sources)
					this.iterate(target, source, false, true);
			}
			else
				this.iterate(target, sources, false, true);
		}
		for (var target in property) {
			var sources = property[target];
			if (target == "~") {
			}
			else {
				target = tool.resolvePrefix(target);
				target = tool.resolveSlash(target);
				var slash = target.lastIndexOf(tool.slash);
				if (slash >= 0)
					this.appendTarget(target.slice(0, slash));
				var star = target.lastIndexOf("*");
				if (star >= 0) {
					var suffix = target.slice(star + 1);
					target = target.slice(0, star);
					if (sources instanceof Array) {
						for (var source of sources) {
							if (typeof source == "string")
								source = tool.resolveSlash(source);
							else
								source.source = tool.resolveSlash(source.source);
							this.iterate(target, source, true, suffix, false);
						}
					}
					else
						this.iterate(target, sources, true, suffix, false);
				}
				else {
					var suffix = ""
					var pipe = target.lastIndexOf("|");
					if (pipe >= 0) {
						var suffix = target.slice(pipe + 1);
						target = target.slice(0, pipe);
					}
					if (sources instanceof Array) {
						for (var source of sources)
							this.iterate(target, source, true, suffix, true);
					}
					else
						this.iterate(target, sources, true, suffix, true);
				}
			}
		}
	}
};

class DataRule extends Rule {
	appendSource(target, source, include, suffix, parts, kind) {
		var tool = this.tool;
		if (kind < 0)
			return;
		this.appendFile(tool.dataFiles, target + parts.extension, source, include);
	}
	appendTarget(target) {
		this.appendFolder(this.tool.dataFolders, target);
	}
	noFilesMatch(source, star) {
		this.tool.reportWarning(null, 0, "no data match: " + source);
	}
};

class BLERule extends Rule {
	appendSource(target, source, include, suffix, parts, kind) {
		if ((parts.extension == ".json") && (parts.directory.endsWith("bleservices"))) {
			this.appendFile(this.tool.bleServicesFiles, parts.name, source, include);
		}
	}
};

class ModulesRule extends Rule {
	appendSource(target, source, include, suffix, parts, kind, query) {
		var tool = this.tool;
		if (kind < 0)
			return;
		if (tool.dataFiles.already[source])
			return;
		if (parts.extension == ".js")
			this.appendFile(tool.jsFiles, target + ".xsb", source, include);
		else if (parts.extension == ".c")
			this.appendFile(tool.cFiles, parts.name + ".c.o", source, include);
		else if (parts.extension == ".cc")
			this.appendFile(tool.cFiles, parts.name + ".cc.o", source, include);
		else if (parts.extension == ".cpp")
			this.appendFile(tool.cFiles, parts.name + ".cpp.o", source, include);
		else if (parts.extension == ".m")
			this.appendFile(tool.cFiles, parts.name + ".m.o", source, include);
		else if (parts.extension == ".java")
			this.appendFile(tool.javaFiles, parts.name + ".class", source, include);
		else if (parts.extension == ".h") {
			this.appendFolder(tool.cFolders, parts.directory);
			this.appendFolder(tool.hFiles, source);
			if ("cdv" === query.transform) {
				const result = this.appendFile(tool.cdvFiles, target, source, include);
				if (result) {
					result.query = {...query};
					delete result.query.source; 
					delete result.query.transform; 
				}
			}
			else if (parts.name.endsWith(".cdv"))
				this.appendFile(tool.cdvFiles, target.slice(0, -4), source, include);
		}
		else if (parts.extension == ".ts") {
			if (parts.name.endsWith(".d")) {
				this.appendFile(tool.dtsFiles, target, source, include);
			}
			else {
				this.appendFile(tool.tsFiles, target + ".xsb", source, include);
			}
		}
		else if (parts.extension == ".json") {
			if ("nodered2mcu" === query.transform)
				this.appendFile(tool.nodered2mcuFiles, target, source, include);
		}
	}
	appendTarget(target) {
		this.appendFolder(this.tool.jsFolders, target);
	}
	noFilesMatch(source, star) {
		this.tool.reportWarning(null, 0, "no modules match: " + source);
	}
};

var moduleExtensions = [
	".c", ".cc", ".cpp", ".h", ".java", ".js", ".json", ".m",
];

class ResourcesRule extends Rule {
	appendBitmap(name, path, include, suffix) {
		var tool = this.tool;
		let colorFile, alphaFile;
		if (tool.bmpAlphaFiles.already[path] || tool.bmpColorFiles.already[path] || tool.bmpMaskFiles.already[path])
			return;
		if (suffix == "-color") {
			colorFile = this.appendFile(tool.bmpColorFiles, name + "-color.bmp", path, include);
		}
		else if (suffix == "-color-monochrome") {
			colorFile = this.appendFile(tool.bmpColorFiles, name + "-color.bm4", path, include);
			colorFile.monochrome = true;
		}
		else if (suffix == "-color-argb4444") {
			colorFile = this.appendFile(tool.bmpColorFiles, name + "-color.bmp", path, include);
			colorFile.format = "argb4444";
		}
		else if (suffix == "-alpha") {
			alphaFile = this.appendFile(tool.bmpAlphaFiles, name + "-alpha.bmp", path, include);
		}
		else if (suffix == "-alpha-monochrome") {
			alphaFile = this.appendFile(tool.bmpAlphaFiles, name + "-alpha.bm4", path, include);
			alphaFile.monochrome = true;
		}
		else if (suffix == "-mask") {
			alphaFile = this.appendFile(tool.bmpMaskFiles, name + "-alpha.bm4", path, include);
		}
		else {
			colorFile = this.appendFile(tool.bmpColorFiles, name + "-color.bmp", path, include);
			alphaFile = this.appendFile(tool.bmpAlphaFiles, name + "-alpha.bmp", path, include);
			alphaFile.colorFile = colorFile;
			colorFile.alphaFile = alphaFile;
		}
		if (colorFile) {
			colorFile.clutName = tool.clutFiles.current;
		}
	}
	appendFont(name, path, include, suffix) {
		var tool = this.tool;
		if (suffix == "-mask") {
			this.appendFile(tool.bmpFontFiles, name + ".bf4", path, include);
			return false;
		}
		this.appendFile(tool.resourcesFiles, name + ".fnt", path, include);
		return true;
	}
	appendFramework(files, target, source, include) {
		this.count++;
		source = this.tool.resolveDirectoryPath(source);
		if (!files.already[source]) {
			files.already[source] = true;
			if (include) {
				if (!files.find(file => file.target == target)) {
					//this.tool.report(target + " " + source);
					let result = { target, source };
					files.push(result);
					return result;
				}
			}
		}
	}
	appendImage(name, path, include, suffix) {
		var tool = this.tool;
		if (tool.imageFiles.already[path])
			return;
		let file = this.appendFile(tool.imageFiles, name + ".cs", path, include);
		let a = suffix.match(/-image(\(([0-9]+)\))?/);
		file.quality = (a && (a.length == 3) && (a[2] !== undefined)) ? parseInt(a[2]) : undefined;
	}
	appendImageDirectory(name, path, include, suffix) {
		var tool = this.tool;
		var files = tool.imageFiles;
		var source = tool.resolveDirectoryPath(path);
		var target = name + ".cs";
		if (files.already[source])
			return;
		files.already[source] = true;
		if (!include)
			return;
		if (files.find(file => file.target == target))
			return;
		let a = suffix.match(/-image(\(([0-9]+)\))?/);
		let quality = (a && (a.length == 3) && (a[2] !== undefined)) ? parseInt(a[2]) : undefined;
		files.push({ target, source, quality });
		this.count++;
	}
	appendOutlineFont(name, path, include, suffix, query) {
		const tool = this.tool;
		let file = this.appendFile(tool.outlineFontFiles, name + ".fnt", path, include);
		if (!file)
			file = tool.outlineFontFiles.find(file => file.source == path);

		const face = {...query, suffix, name: query.name ?? name};
		if (!face.size)
			throw new Error(`missing required font size for "${face.name}"`);

		if (file.faces?.find(item => (item.size === face.size) && (item.name === face.name))) 
			throw new Error(`duplicate font "${face.name}", size ${face.size}`);

		file.faces ??= [];
		file.faces.push(face);
	}
	appendSound(name, path, include, suffix) {
		var tool = this.tool;
		this.appendFile(tool.soundFiles, name + ".maud", path, include);
	}
	appendSource(target, source, include, suffix, parts, kind, query) {
		var tool = this.tool;
		if (kind < 0) {
			if (tool.format) {
				for (var fps = 1; fps <= 60; fps++) {
					var extension = "." + fps + "fps";
					if (parts.extension == extension) {
						this.appendImageDirectory(target, source, include, suffix);
						return;
					}
				}
			}
			if (tool.platform == "x-ios" && (parts.extension == ".framework" || parts.extension == ".bundle")) {
				this.appendFramework(tool.resourcesFiles, target + parts.extension, source, include);
				return;
			}
			return;
		}
		if (tool.dataFiles.already[source])
			return;
		if ((parts.extension == ".json") && (parts.directory.endsWith("strings"))) {
			this.appendFile(tool.stringFiles, tool.localsName + "." + parts.name + ".mhr", source, include);
			return;
		}
		if (tool.format) {
			if (parts.extension == ".act") {
				this.appendFile(tool.clutFiles, target + ".cct", source, include);
				tool.clutFiles.current = target;
				return;
			}
			if (parts.extension == ".fnt") {
				this.appendFont(target, source, include, suffix);
				return;
			}
			if (parts.extension == ".png") {
				if (suffix.startsWith("-image")) {
					this.appendImage(target, source, include, suffix);
					return;
				}
				parts.extension = ".fnt";
				if (!tool.bmpFontFiles.already[tool.joinPath(parts)])
					this.appendBitmap(target, source, include, suffix);
				return;
			}
			if ((parts.extension == ".gif") || (parts.extension == ".jpeg") || (parts.extension == ".jpg")) {
				this.appendImage(target, source, include, suffix);
				return;
			}
			if (parts.extension == ".wav") {
				this.appendSound(target, source, include, suffix);
				return;
			}
			if ((parts.extension == ".ttf") || (parts.extension == ".otf")) {
				this.appendOutlineFont(target, source, include, suffix, query);
				return;
			}
		}
		if (moduleExtensions.indexOf(parts.extension) >= 0)
			return;
		this.appendFile(tool.resourcesFiles, target + parts.extension, source, include);
	}
	appendTarget(target) {
		this.appendFolder(this.tool.resourcesFolders, target);
	}
	noFilesMatch(source) {
		this.tool.reportWarning(null, 0, "no resources match: " + source);
	}
};

export class Tool extends TOOL {
	constructor(argv) {
		super(argv);
		if (this.currentPlatform == "wasm") {
			this.moddablePath = "/moddable";
			this.createDirectory(this.moddablePath);
			this.createDirectory(this.moddablePath + "/build");
		}
		else {
			this.moddablePath = this.getenv("MODDABLE");
			if (!this.moddablePath)
				throw new Error("MODDABLE: variable not found!");
		}
		this.config = {};
		this.debug = false;
		this.environment = { "MODDABLE": this.moddablePath }
		this.format = null;
		this.instrument = false;
		this.mainPath = null;
		this.make = false;
		this.manifestPath = null;
		this.outputPath = null;
		this.platform = null;
		this.rotation = undefined;
		this.signature = null;
		this.verbose = false;
		this.windows = this.currentPlatform == "win";
		this.slash = this.windows ? "\\" : "/";

		this.buildPath = this.moddablePath + this.slash + "build";
		this.xsPath = this.moddablePath + this.slash + "xs";

		var name, path;
		var argc = argv.length;
		for (var argi = 1; argi < argc; argi++) {
			var option = argv[argi];
			switch (option) {
			case "-d":
				this.debug = true;
				this.instrument = true;
				break;
			case "-f":
				argi++;
				if (argi >= argc)
					throw new Error("-f: no format!");
				name = argv[argi];
				if (this.format)
					throw new Error("-f '" + name + "': too many formats!");
				name = name.toLowerCase();
				if (name in formatNames)
					name = formatNames[name];
				else
					throw new Error("-f '" + name + "': unknown format!");
				this.format = name;
				break;
			case "-i":
				this.instrument = true;
				break;
			case "-m":
				this.make = true;
				break;
			case "-o":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no directory!");
				name = argv[argi];
				if (this.outputPath)
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
				name = name.toLowerCase();
				let parts = name.split("/");
				if ("esp8266" === parts[0])
					parts[0] = "esp";
				else if ((parts[0] == "sim") || (parts[0] == "simulator"))
					parts[0] = this.currentPlatform;
				this.platform = parts[0];
				if (parts[1]) {
					this.subplatform = parts[1];
					this.environment.SUBPLATFORM = this.subplatform;
					this.fullplatform = this.platform + "/" + this.subplatform;
					this.environment.PLATFORMPATH = this.platform + this.slash + this.subplatform;
				}
				else {
					this.fullplatform = this.platform;
					this.environment.PLATFORMPATH = this.platform;
				}
				this.environment.PLATFORM = this.platform;
				this.environment.FULLPLATFORM = this.fullplatform;
				break;
			case "-r":
				argi++;
				if (argi >= argc)
					throw new Error("-r: no rotation!");
				if (undefined !== this.rotation)
					throw new Error("-r '" + name + "': too many rotations!");
				name = parseInt(argv[argi]);
				if ((name != 0) && (name != 90) && (name != 180) && (name != 270))
					throw new Error("-r: " + name + ": invalid rotation!");
				this.rotation = name;
				break;
			case "-s":
				argi++;
				if (argi >= argc)
					throw new Error("-s: no signature!");
				if (null !== this.signature)
					throw new Error("-s '" + name + "': too many signatures!");
				name = argv[argi];
				if (name.split('.').length != 3)
					throw new Error("-s: " + name + ": invalid signature!");
				this.signature = name;
				break;
			case "-v":
				this.verbose = true;
				break;
			case "-t":
				if (++argi >= argc)
					throw new Error("-t: no build target!");
				if (undefined === this.buildTarget)
					this.buildTarget = argv[argi];
				else
					this.buildTarget = this.buildTarget + " " + argv[argi];
				break;
			case "-x":
				argi++;
				if (argi >= argc)
					throw new Error("-x: no host");
				name = argv[argi];
				if (undefined !== this.xsbug)
					throw new Error("-x '" + name + "': only one!");
				name = name.split(":");
				this.xsbug = {
					host: name[0],
					port: name[1]
				};
				break;
			case "-l":
				this.xsbugLog = true;
				break;
			default:
				name = argv[argi];
				let split = name.split("=");
				if (split.length == 2) {
					this.config[split[0]] = split[1];
				}
				else {
					if (this.manifestPath)
						throw new Error("'" + name + "': too many manifests!");
					path = this.resolveFilePath(name);
					if (!path)
						throw new Error("'" + name + "': manifest not found!");
					this.manifestPath = path;
				}
				break;
			}
		}
		if (!this.fullplatform) {
			this.fullplatform = this.platform = this.currentPlatform;
			this.environment.PLATFORM = this.platform;
			this.environment.FULLPLATFORM = this.platform;
			this.environment.PLATFORMPATH = this.platform;
		}
		path = this.environment.MODDABLE + this.slash + "modules" + this.slash + "network" + this.slash + "ble" + this.slash;
		if ("esp32" == this.platform) {
			let bluedroid = this.getenv("ESP32_BLUEDROID") === "1";
			path += bluedroid ? this.platform : "nimble";
/**/			let subclass = this.getenv("ESP32_SUBCLASS");
/**/			if (undefined === subclass)
/**/				subclass = "esp32";
/**/			this.environment.ESP32_SUBCLASS = subclass;
		}
		else if ("mac" == this.platform || "win" == this.platform || "lin" == this.platform)
			path += "sim";
		else
			path += this.platform;
		this.environment.BLEMODULEPATH = path;

		let userHome;
		if ("win" == this.currentPlatform){
			userHome = this.getenv("USERPROFILE");
		}else if ("mac" == this.currentPlatform || "lin" == this.currentPlatform){
			userHome = this.getenv("HOME");
		}
		if (userHome !== undefined) this.environment.USERHOME = userHome; 

		if (this.manifestPath) {
			var parts = this.splitPath(this.manifestPath);
			this.currentDirectory = this.mainPath = parts.directory;
		}
		else {
			path = this.resolveFilePath("." + this.slash + "manifest.json");
			if (path)
				this.manifestPath = path;
			else {
				path = this.resolveFilePath(".." + this.slash + "manifest.json");
				if (path)
					this.manifestPath = path;
				else
					throw new Error("no manifest!");
			}
			this.mainPath = this.currentDirectory;
		}
		var parts = this.splitPath(this.mainPath);
		this.environment.NAME = parts.name + parts.extension;
		if (!this.outputPath)
			this.outputPath = this.buildPath;
		if (!this.platform)
			this.platform = this.currentPlatform;
		if (this.platform.startsWith("x-"))
			this.format = null;
		else if (!this.format)
			this.format = "UNDEFINED";
		if (this.platform == "mac")
			this.environment.SIMULATOR = this.moddablePath + "/build/bin/mac/debug/mcsim.app";
		else if (this.platform == "win")
			this.environment.SIMULATOR = this.moddablePath + "\\build\\bin\\win\\debug\\mcsim.exe";
		else if (this.platform == "lin")
			this.environment.SIMULATOR = this.moddablePath + "/build/bin/lin/debug/mcsim";
		this.environment.BUILD_SIMULATOR = this.moddablePath + this.slash + "build" + this.slash + "simulators";
	}
	concatProperties(object, properties, flag) {
		if (properties) {
			for (let name in properties) {
				let property = properties[name];
				if (flag) {
					if (property instanceof Array)
						property = property.map(item => this.resolveSource(item));
					else if (typeof property == "string")
						property = this.resolveSource(property);
				}
				object[name] = this.concatProperty((name in object) ? object[name] : [], property);
			}
		}
	}
	concatProperty(array, value) {
		if ((value instanceof Array) || (typeof value == "string"))
			return array.concat(value);
		return array;
	}
	createFolder(path, folder) {
		const names = folder.split(this.slash);
		for (let name of names) {
			path += this.slash + name;
			this.createDirectory(path)
		}
	}
	includeManifest(it) {
		var currentDirectory = this.currentDirectory;
		if ("string" == typeof it) {
			this.includeManifestPath(this.resolveVariable(it));
		}
		else if (this.buildTarget != "clean") {
			let { git, branch, tag, include = "manifest.json" } = it;
			if (!git)
				throw new Error("no git!");
			let repo = this.resolveVariable(git);
			if (this.windows)
				repo = repo.replace(/\\/g, "/");
			let url = new URL(repo);
			
			let directory = "repos/" + url.hostname + url.pathname;
			if (directory.endsWith(".git"))
				directory = directory.slice(0, -4);
			if (branch)
				directory += "/" + branch;
			if (tag)
				directory += "/" + tag;
			
			let parts = directory.split("/");
			let path = this.createDirectories(this.outputPath, "tmp", this.environment.NAME);
			directory = path + this.slash + parts.join(this.slash);
			
			if (this.isDirectoryOrFile(directory) == 0) {
				for (let part of parts) {
					path += this.slash + part;
					this.createDirectory(path);
				}
				this.currentDirectory = path;
				this.report("# git clone " + repo + " to path " + path);
				let result;
				if (branch)
					result = this.spawn("git", "clone", "-b", branch, repo, ".");
				else
					result = this.spawn("git", "clone", repo, ".");
				if (result != 0)
					throw new Error("git failed!");
				if (tag) {
					result = this.spawn("git", "-c", "advice.detachedHead=false", "checkout", tag);
					if (result != 0)
						throw new Error("git failed!");
				}
			}
// 			else {
// 				this.currentDirectory = directory;
// 				this.report("# git pull " + name);
// 				this.spawn("git", "pull");
// 			}
			if (include instanceof Array)
				include.forEach(it => this.includeManifestPath(directory + this.slash + this.resolveVariable(it)));
			else
				this.includeManifestPath(directory + this.slash + this.resolveVariable(include));
		}
		this.currentDirectory = currentDirectory;
	}
	includeManifestPath(include) {
		let path = this.resolveFilePath(include);
		if (!path)
			throw new Error("'" + include + "': manifest not found!");
		if (!this.manifests.already[path]) {
			var parts = this.splitPath(path);
			this.currentDirectory = parts.directory;
			var manifest = this.parseManifest(path);
			manifest.directory = parts.directory;
		}
	}
	matchPlatform(platforms, name, simple) {
		let parts = name.split("/");
		while (parts.length) {
			let partial = parts.join("/");
			for (let n in platforms) {
				if (n.endsWith("/*")) {
					let head = n.slice(0, -1);
					if (partial.startsWith(head))
						return platforms[n];
				}
				else if (partial === n)
					return platforms[n];
			}
			parts.length -= 1;
		}

		if (simple)
			return;

		for (let n in platforms) {
			if ("..." === n)
				return platforms[n];
		}
	}
	mergeManifest(all, manifest) {
		var currentDirectory = this.currentDirectory;
		this.currentDirectory = manifest.directory;
		this.mergePlatform(all, manifest);

		if ("platforms" in manifest) {
			let platforms = manifest.platforms;
			let platform = this.matchPlatform(platforms, this.fullplatform, false);
			if (platform)
				this.mergePlatform(all, platform);
			delete manifest.platforms;
		}
		this.currentDirectory = currentDirectory;
	}
	mergeNodeRed(manifests) {
		manifests.forEach(manifest => {
			const modules = manifest.modules?.["*"];
			if (!modules) return;

			for (const specifier in modules) {
				const module = modules[specifier];
				if ("object" !== typeof module)
					continue;
				if ("nodered2mcu" !== module.transform)
					return;

				this.currentDirectory = manifest.directory;
				const source = this.resolveFilePath(this.resolveVariable(module.source) + ".json");
				const flows = JSON.parse(this.readFileString(source));
				flows.forEach((node, i) => {
					if (node.moddable_manifest)
						this.parseManifest(source, {...node.moddable_manifest, directory: this.currentDirectory});
				});
			}
		});
	}
	mergePlatform(all, platform) {
		this.mergeProperties(all.config, platform.config);
		this.mergeProperties(all.creation, platform.creation);
		this.mergeProperties(all.defines, platform.defines);

		this.concatProperties(all.data, platform.data, true);
		this.concatProperties(all.modules, platform.modules, true);
		this.concatProperties(all.resources, platform.resources, true);
		this.concatProperties(all.ble, platform.ble, true);
		this.concatProperties(all.recipes, platform.recipes);

		all.commonjs = this.concatProperty(all.commonjs, platform.commonjs);
		all.preload = this.concatProperty(all.preload, platform.preload);
		all.strip = platform.strip ? platform.strip : all.strip;
		all.errors = this.concatProperty(all.errors, platform.error);
		all.warnings = this.concatProperty(all.warnings, platform.warning);
		this.mergeProperties(all.run, platform.run);
		if (platform.typescript) {
			let tsconfig = platform.typescript.tsconfig;
			if (tsconfig) {
				tsconfig = JSON.parse(JSON.stringify(tsconfig), (name, value) => {
					if ("string" === typeof value)
						value = this.resolveVariable(value);
					return value;
				});

				const compilerOptions = tsconfig.compilerOptions;
				for (let name in compilerOptions) {
					let value = compilerOptions[name];
					if ("types" === name)
						value = value.map(path => this.resolveSource(path));
					if ("object" === typeof value) {
						if (value instanceof Array) {
							all.typescript.tsconfig.compilerOptions[name] ??= [];
							all.typescript.tsconfig.compilerOptions[name] = value.concat(all.typescript.tsconfig.compilerOptions[name]); 
						}
						else {
							all.typescript.tsconfig.compilerOptions[name] ??= {};
							this.mergeProperties(all.typescript.tsconfig.compilerOptions[name], value);
						}
					}
					else
						all.typescript.tsconfig.compilerOptions[name] = value;
				}
			}

			all.typescript.compiler = platform.typescript.compiler ?? all.typescript.compiler;
		}
		return;
	}
	mergeProperties(targets, sources) {
		if (sources) {
			for (let name in sources) {
				let target = targets[name];
				let source = sources[name];
				if (target && source && (typeof target == "object") && (typeof source == "object"))
					this.mergeProperties(target, source);
				else
					targets[name] = source;
			}
		}
	}
	parseBuild(platform) {
		let properties = platform.build;
		if (properties) {
			for (let name in properties) {
				let value = properties[name];
				if (typeof value == "string") {
					value = this.resolveVariable(value);
					if (value.startsWith("./")) {
						const path = this.resolveDirectoryPath("./");
						if (path) {
							if ("./" == value)
								value = path;
							else
								value = path + value.slice(1);
						}
					}
					this.environment[name] = value;
				}
				else
					this.environment[name] = value;
			}
		}
	}
	parseManifest(path, manifest) {
		let platformInclude;
		if (!manifest) {
			var buffer = this.readFileString(path);
			try {
				var manifest = JSON.parse(buffer);
			}
			catch (e) {
				var message = e.toString();
				var result = /SyntaxError: ([^:]+: )?([0-9]+): (.+)/.exec(message);
				if (result.length == 4) {
					this.reportError(path, parseInt(result[2]), result[3]);
				}
				throw new Error("'" + path + "': invalid manifest!");;
			}
		}
		this.manifests.already[path] = manifest;
		this.parseBuild(manifest);
		if ("platforms" in manifest) {
			let platforms = manifest.platforms;
			let platform = this.matchPlatform(platforms, this.fullplatform, false);
			if (platform) {
				this.parseBuild(platform);
				platformInclude = platform.include;
				if (platformInclude) {
					if (!("include" in manifest))
						manifest.include = platformInclude;
					else {
						if ("string" === typeof manifest.include)
							manifest.include = [manifest.include];
						manifest.include = manifest.include.concat(platformInclude);
					}
				}
			}
		}
		if ("include" in manifest) {
			if (manifest.include instanceof Array)
				manifest.include.forEach(include => this.includeManifest(include));
			else
				this.includeManifest(manifest.include);
		}
		this.manifests.push(manifest);
		return manifest;
	}
	resolvePrefix(value) {
		const colon = value.indexOf(":");
		if (colon > 0) {
			value = "~." + value.slice(0, colon) + "/" + value.slice(colon + 1);
		}
		return value;
	}
	resolveSlash(value) {
		if (this.windows)
			value = value.replace(/\//g, "\\");
		return value;
	}
	resolveSource(sourceIn) {
		var source = ("string" == typeof sourceIn) ? sourceIn : sourceIn.source;
		var result = this.resolveVariable(source);
		var slash = result.lastIndexOf(this.slash);
		if (slash < 0)
			throw new Error("'" + source + "': path not found!");
		var directory = this.resolveDirectoryPath(result.slice(0, slash));
		if (!directory)
			throw new Error("'" + source + "': directory not found!");
		result = directory + result.slice(slash);
		if ("string" == typeof sourceIn)
			return result;
		return {...sourceIn, source: result};
	}
	resolveVariable(value) {
		value = value.replace(/\$\(([^\)]+)\)/g, (offset, value) => {
			if (value in this.environment)
				return this.environment[value];
			return this.getenv(value);
		});
		return this.resolveSlash(value);
	}
	run() {
		// merge manifests
		var path = this.manifestPath;
		this.manifests = [];
		this.manifests.already = {};
		var manifest = this.parseManifest(this.manifestPath);
		manifest.directory = this.mainPath;

		this.mergeNodeRed(this.manifests);

		this.manifest = {
			config:{},
			creation:{},
			defines:{},
			data:{},
			modules:{},
			resources:{},
			ble:{},
			recipes:{},
			preload:[],
			strip:[],
			commonjs:[],
			errors:[],
			warnings:[],
			run:{},
			typescript: {compiler: "tsc", tsconfig: {compilerOptions: {}}}
		};
		this.manifests.forEach(manifest => this.mergeManifest(this.manifest, manifest));

		if (this.manifest.errors.length) {
			this.manifest.errors.forEach(error => { this.reportError(null, 0, error); });
			throw new Error("incompatible platform!");
		}
		if (this.manifest.warnings.length) {
			this.manifest.warnings.forEach(warning => { this.reportWarning(null, 0, warning); });
		}

		// apply rules
		this.dataFiles = [];
		this.dataFiles.already = {};
		this.dataFolders = [];
		this.dataFolders.already = {};

		this.jsFiles = [];
		this.jsFiles.already = {};
		this.jsFolders = [];
		this.jsFolders.already = {};

		this.cFiles = [];
		this.cFiles.already = {};
		this.cFolders = [];
		this.cFolders.already = {};
		this.hFiles = [];
		this.hFiles.already = {};
		this.javaFiles = [];
		this.javaFiles.already = {};
		
		this.tsFiles = [];
		this.tsFiles.already = {};
		this.dtsFiles = [];
		this.dtsFiles.already = {};
		
		this.cdvFiles = [];
		this.cdvFiles.already = {};
		
		this.nodered2mcuFiles = [];
		this.nodered2mcuFiles.already = {};

		this.resourcesFiles = [];
		this.resourcesFiles.already = {};
		this.resourcesFolders = [];
		this.resourcesFolders.already = {};

		this.bmpColorFiles = [];
		this.bmpColorFiles.already = {};
		this.bmpAlphaFiles = [];
		this.bmpAlphaFiles.already = {};
		this.bmpFontFiles = [];
		this.bmpFontFiles.already = {};
		this.bmpMaskFiles = [];
		this.bmpMaskFiles.already = {};
		this.clutFiles = [];
		this.clutFiles.already = {};
		this.clutFiles.current = "";
		this.imageFiles = [];
		this.imageFiles.already = {};
		this.outlineFontFiles = [];
		this.outlineFontFiles.already = {};
		this.soundFiles = [];
		this.soundFiles.already = {};
		this.stringFiles = [];
		this.stringFiles.already = {};
		this.bleServicesFiles = [];
		this.bleServicesFiles.already = {};

		var rule = new DataRule(this);
		rule.process(this.manifest.data);
		var rule = new ModulesRule(this);
		rule.process(this.manifest.modules);
		var rule = new ResourcesRule(this);
		rule.process(this.manifest.resources);
		var rule = new BLERule(this);
		rule.process(this.manifest.ble);
		
		if (this.signature == null) {
			if (!this.environment.NAMESPACE)
				this.environment.NAMESPACE = "moddable.tech"
			this.signature = this.environment.NAME + "." + this.environment.NAMESPACE;
		}
		var signature = this.signature.split(".").reverse();
		this.environment.DASH_SIGNATURE = signature.join("-");
		this.environment.DOT_SIGNATURE = signature.join(".");
		this.environment.SLASH_SIGNATURE = "/" + signature.join("/");
	}
	unresolvePrefix(value) {
		if (value.startsWith("~.")) {
			value = value.slice(2).replace("/", ":");
		}
		return value;
	}
}

