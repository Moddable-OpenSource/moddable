/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
import { URL } from "url";

const defaultConditions = ["moddable", "import"];

const consoleGlobal = {
	snippet: `
globalThis.console = Object.freeze({
	log(...args) {
		trace(...args);
		trace("\\n");
	}
});
`
};
const fetchGlobal = {
	include: "$(MODDABLE)/examples/io/tcp/fetch/manifest_fetch.json",
	snippet: `
import { fetch } from "fetch";
globalThis.fetch = fetch;
`	
};
const headersGlobal = {
	include: "$(MODDABLE)/modules/data/headers/manifest.json",
	snippet: `
import Headers from "headers";
globalThis.Headers = Headers;
`	
};
const structuredCloneGlobal = {
	include: "$(MODDABLE)/modules/base/structuredClone/manifest.json",
	snippet: `
import structuredClone from "structuredClone";
globalThis.structuredClone = structuredClone;
`	
};
const textDecoderGlobal = {
	include: "$(MODDABLE)/modules/data/text/decoder/manifest.json",
	snippet: `
import TextDecoder from "text/decoder";
globalThis.TextDecoder = TextDecoder;
`	
};
const textEncoderGlobal = {
	include: "$(MODDABLE)/modules/data/text/encoder/manifest.json",
	snippet: `
import TextEncoder from "text/encoder";
globalThis.TextEncoder = TextEncoder;
`	
};
const timerGlobal = {
	snippet: `
import Timer from "timer";
globalThis.clearImmediate = globalThis.clearInterval = globalThis.clearTimeout = function(id) { return Timer.clear(id) };
globalThis.setImmediate = function(callback) { return Timer.set(callback) };
globalThis.setInterval = function(callback, delay) { return Timer.repeat(callback, delay) };
globalThis.setTimeout = function(callback, delay) { return Timer.set(callback, delay) };
`	
};
const urlGlobal = {
	include: "$(MODDABLE)/modules/data/url/manifest.json",
	snippet: `
import { URL, URLSearchParams } from "url";
globalThis.URL = URL;
globalThis.URLSearchParams = URLSearchParams;
`	
};
const workerGlobal = {
	include: "$(MODDABLE)/modules/base/worker/manifest.json",
	snippet: `
import Worker from "worker";
globalThis.Worker = Worker;
import {SharedWorker} from "worker";
globalThis.SharedWorker = SharedWorker;
`	
};

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.moddablePath = this.getenv("MODDABLE");
		if (!this.moddablePath)
			throw new Error("MODDABLE: variable not found!");
		this.windows = this.currentPlatform == "win";
		this.slash = this.windows ? "\\" : "/";
		this.buildPath = this.moddablePath + this.slash + "build";
		this.packagePath = this.currentDirectory;
		this.examplesPath = this.moddablePath + this.slash + "examples";
		this.modulesPath = this.moddablePath + this.slash + "modules";
		this.outputPath = null;
		this.environment = {
			MODDABLE:this.moddablePath,
			MODULES:this.modulesPath,
			COMMODETTO:this.modulesPath + this.slash + "commodetto",
		};
		
		let name, path;
		let argc = argv.length;
		let argi = 1;
		for (; argi < argc; argi++) {
			name = argv[argi];
			if ((name == "mcconfig") || (name == "mcrun") || (name == "list"))
				break;
			path = this.resolveFilePath(name);
			if (!path)
				throw new Error("" + name + "': package not found!");
			let parts = this.splitPath(path);
			if ((parts.name != "package") || (parts.extension != ".json"))
				throw new Error("" + name + "': invalid package name!");
			this.packagePath = parts.directory;
			this.currentDirectory = this.packagePath;
		}
		if (name == "mcconfig") {
			this.hasCreationMain = true;
		}
		else if (name == "mcrun") {
			this.hasCreationMain = false;
		}
		else if (name == "list") {
			this.hasCreationMain = undefined;
		}
		else {
			throw new Error("no action!");
		}
		
		argi++;
		this.argv = argv.slice(argi);
		for (; argi < argc; argi++) {
			var option = argv[argi];
			switch (option) {
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
		
		if (this.platform == "mac")
			this.environment.SIMULATOR = `${this.moddablePath}/build/bin/mac/${this.build}/mcsim.app`;
		else if (this.platform == "win")
			this.environment.SIMULATOR = `${this.moddablePath}\\build\\bin\\win\\${this.build}\\mcsim.exe`;
		else if (this.platform == "lin")
			this.environment.SIMULATOR = `${this.moddablePath}/build/bin/lin/${this.build}/mcsim`;
			
		this.environment.BUILD_SIMULATOR = this.moddablePath + this.slash + "build" + this.slash + "simulators";
		
		if (!this.outputPath)
			this.outputPath = this.buildPath;
		
		this.verbose = this.argv.indexOf("-v") >= 0;
		
		this.includes = {
			"piu/MC": "$(MODDABLE)/examples/manifest_piu.json",
		}
		this.globals = {
			"console": consoleGlobal,
			"fetch": fetchGlobal,
			"Headers": headersGlobal,
			"structuredClone": structuredCloneGlobal,
			"TextDecoder": textDecoderGlobal,
			"TextEncoder": textEncoderGlobal,
			"clearImmediate": timerGlobal,
			"clearInterval": timerGlobal,
			"clearTimeout": timerGlobal,
			"setImmediate": timerGlobal,
			"setInterval": timerGlobal,
			"setTimeout": timerGlobal,
			"URL": urlGlobal,
			"URLSearchParams": urlGlobal,
			"Worker": workerGlobal,
			"SharedWorker": workerGlobal,
		}
		this.snippets = [];
	}
	createDirectories(path, ...segments) {
		for (let segment of segments) {
			path += this.slash + segment;
			this.createDirectory(path);
		}
		return path;
	}
	readFileJSON(path) {
		let buffer = this.readFileString(path);
		try {
			return JSON.parse(buffer);
		}
		catch (e) {
			let message = e.toString();
			let result = /SyntaxError: ([^:]+: )?([0-9]+): (.+)/.exec(message);
			if (result.length == 4) {
				this.reportError(path, parseInt(result[2]), result[3]);
			}
			throw e;
		}
	}
	writeDirectoryFileString(path, name, string) {
		let segments = name.split('/');
		if (segments.length > 1)
			path = this.createDirectories(path, ...segments.slice(0, -1));
		path += this.slash + segments[segments.length - 1];
		this.writeFileString(path, string);
	}
	
	listSpecifiers(path) @ "Tool_prototype_listSpecifiers"
	parseModule(module) {
		let path = this.urlToFilePath(module.fileURL);
		let infos = this.listSpecifiers(path);
		if (infos) {
			module.async = infos.async;
			module.default = infos.default;
			for (let specifier of infos.from) {
				try {
					let from = specifier.from;
					let fileURL = this.ESM_RESOLVE(from, module.fileURL);
					if (fileURL.startsWith("embedded:") || fileURL.startsWith("moddable:")) {
						if (this.importedBuiltinSpecifiers.indexOf(fileURL) < 0)
							this.importedBuiltinSpecifiers.push(fileURL)
					}
					else {
						let packageURL = this.LOOKUP_PACKAGE_SCOPE(fileURL);
						if (packageURL == null)
							this.throwPackageNotFoundError();
						let packageJSON = this.READ_PACKAGE_JSON(packageURL);
						if (packageJSON == null)
							this.throwPackageNotFoundError();
						let packageName = packageJSON.name;
						if (from.startsWith('/') || from.startsWith('./') || from.startsWith('../')) {
							if (fileURL.indexOf(packageURL) == 0)
								from = packageName + "/" + fileURL.slice(packageURL.length + 1);
							else
								this.throwInvalidModuleSpecifierError();
						}
						let item = this.modules.get(fileURL);
						if (item) {
							if (!item.specifiers.find(it => it === from))
								item.specifiers.push(from);
						}
						else {
							item = { fileURL, packageURL, packageName, specifiers: [ from ] };
							this.modules.set(fileURL, item);
							this.queue.push(item);
						}
					}
				}
				catch(e) {
					this.reportError(path, specifier.line, e.message);
				}
			}
			for (let string of infos.global) {
				let global = this.globals[string];
				if (global) {
					let include = global.include;
					if (include) {
						if (this.hasCreationMain) {
							if (this.manifest.include.indexOf(include) < 0) {
								this.report(`# mcpack include: ${include}`);
								this.manifest.include.push(include);
							}
						}
						else {
							if (this.require.indexOf(include) < 0) {
								this.report(`# host must include: ${include}`);
								this.require.push(include);
							}
						}
					}
					let snippet = global.snippet;
					if (snippet) {
						if (this.snippets.indexOf(snippet) < 0) {
							let flag = true;
							let message = (this.hasCreationMain) ? "# mcpack define: " : "# host must define: ";
							for (let key in this.globals) {
								if (this.globals[key] == global) {
									if (flag)
										flag = false;
									else
										message += ", ";
									message += key;
								}
							}
							this.report(message);
							this.snippets.push(snippet);
						}
					}
				}
			}
		}
	}
	run() {
		if (this.hasCreationMain === undefined) {
			const builtins = this.getPlatformBuiltins();
			for (let builtin of builtins) {
				this.report(builtin.specifier);
				for (let manifest of builtin.manifests) {
					this.report("\t" + manifest);
				}
			}
		}
		this.builtins = this.mapBuiltins(this.getPlatformBuiltins());
		if (this.hasCreationMain) {
			const specifier = "moddable:mc/config";
			this.builtins.set(specifier, { specifier, manifests:[] });
		}
		this.importedBuiltinSpecifiers = [];

		this.manifests = new Map();
		this.modules = new Map();
		this.packages = new Map();
		this.queue = [];
		let packageURL = this.pathToURL(this.packagePath);
		let packageName = null;
		let fileURL = null;
		this.manifest = {
			include: [],
			modules: {}
		};
		if (this.hasCreationMain)
			this.manifest.include.push("$(MODDABLE)/examples/manifest_base.json");
		else
			this.require = [ "$(MODDABLE)/examples/manifest_base.json" ];
		try {
			let packageJSON = this.READ_PACKAGE_JSON(packageURL);
			if (packageJSON == null)
				this.throwPackageNotFoundError();
			packageName = packageJSON.name;
			if ((packageJSON.exports !== null) & (packageJSON.exports !== undefined))
				fileURL = this.PACKAGE_EXPORTS_RESOLVE(packageURL, ".", packageJSON.exports, defaultConditions);
			else if (typeof packageJSON.main === "string")
				fileURL = this.mergeURL(packageJSON.main, packageURL + "/package.json");
			if (fileURL)
				fileURL = this.ESM_RESOLVE(fileURL, packageURL);
			else
				this.throwModuleNotFoundError();
		}
		catch(e) {
			this.reportError(null, 0, e.message);
		}
		if (this.errorCount == 0) {
			let item = { fileURL, packageURL, packageName, specifiers: [] };
			this.modules.set(fileURL, item);
			this.queue.push(item);
			while (this.queue.length) {
				this.parseModule(this.queue.shift());
			}
		}
		let mainURL = fileURL;
		let tmpPath = this.createDirectories(this.buildPath, "tmp", ...packageName.split('/'));
		if (this.hasCreationMain && this.snippets.length) {
			let source = this.snippets.join("");
			let path = tmpPath + this.slash + "globals.js";
			this.writeFileString(path, source);
			this.manifest.modules.globals = "./globals";
			this.manifest.preload = [ "globals" ];
		}
		if (this.errorCount == 0) {
			let async = false;
			this.modules.forEach(item => {
				
				let fileURL = item.fileURL;
				let packageName = item.packageName;
				let packageURL = item.packageURL;
				if (this.verbose)
					this.report(`### ${ fileURL } ${ packageURL }`);
				
				let manifestPath = this.manifests.get(packageURL);
				if (manifestPath === undefined) {
					let manifestURL = packageURL + "/manifest.json";
					manifestPath = this.urlToFilePath(manifestURL);
					if (manifestPath) {
						manifestPath = manifestURL.slice(this.windows ? 8 : 7);
						this.manifest.include.unshift(manifestPath);
					}
					let pjson = this.READ_PACKAGE_JSON(packageURL);
					if ("moddable" in pjson) {
						if ("manifest" in pjson.moddable) {
							this.writeDirectoryFileString(this.urlToDirectoryPath(packageURL), `package-manifest.json`, JSON.stringify(pjson.moddable.manifest, null, "\t"));
							manifestURL = packageURL + "/package-manifest.json";
							this.manifest.include.unshift(manifestURL.slice(this.windows ? 8 : 7));
						}
					}
					this.manifests.set(packageURL, manifestPath);
				}
				
				let from = packageName + "/" + fileURL.slice(packageURL.length + 1);
				let source = fileURL.slice(this.windows ? 8 : 7);
				let target = from.slice(0, from.lastIndexOf('.'));
				item.specifiers = item.specifiers.filter(specifier => {
					if (specifier == from) {
						this.manifest.modules[target] = `${source}`;
						source = null;
						return false;
					}
					return true;
				});
				item.specifiers.forEach(specifier => {
					let target;
					if (specifier.startsWith('#'))
						target = packageName + "/" + specifier;
					else
						target = specifier;
					if (source) {
						this.manifest.modules[target] = `${source}`;
						source = null;
					}
					else { //@@ alias
						let string = `export * from "${from}";`;
						if (item.default)
							string += ` import _ from "${from}"; export default _;`;
						this.writeDirectoryFileString(tmpPath, `${target}.js`, string);
						this.manifest.modules[target] = `./${target}.js`;
					}
				});
				if (source) {
					this.manifest.modules[target] = `${source}`;
					source = null;
				}
				if (this.hasCreationMain && (fileURL == mainURL)) {
					this.manifest.creation = { main: target };
// 					if (item.async)
// 						this.manifest.defines = { main: { async: 1 } };
				}
				async |= item.async;
			});
			if (this.hasCreationMain && async)
				this.manifest.defines = { main: { async: 1 } };
			
			const currentDirectory = this.currentDirectory;
			this.manifests = [];
			this.manifests.already = {};
			this.manifest.include.forEach(include => this.includeManifest(include, include, tmpPath));
			const packageBuiltins = this.mapBuiltins(this.getBuiltins(this.manifests));
			this.currentDirectory = currentDirectory;
			
			this.importedBuiltinSpecifiers = this.importedBuiltinSpecifiers.filter(specifier => {
				if (packageBuiltins.get(specifier))
					return false;
				const builtin = this.builtins.get(specifier);
				if (builtin) {
					if (builtin.manifests.length == 0)
						return false;
					if (builtin.manifests.length == 1) {
						const include = "$(MODDABLE)" + builtin.manifests[0]; 
						if (this.hasCreationMain) {
							if (this.manifest.include.indexOf(include) < 0) {
								this.report(`# mcpack include: ${include}`);
								this.manifest.include.push(include);
							}
						}
						else {
							if (this.require.indexOf(include) < 0) {
								this.report(`# host must include: ${include}`);
								this.require.push(include);
							}
						}
						return false;
					}
					return true;
				}
				this.reportError(null, 0, `${specifier}: manifest not found`);
				return false;
			});
			this.importedBuiltinSpecifiers.forEach(specifier => {
				const builtin = this.builtins.get(specifier);
				const index = builtin.manifests.findIndex(manifest => {
					const include = "$(MODDABLE)" + manifest;
					const includes = (this.hasCreationMain) ? this.manifest.include : this.require;
					return includes.indexOf(include) >= 0;
				});
				if (index < 0)
					this.reportError(null, 0, `${specifier}: multiple manifests`);
			});
		}
		if (this.errorCount == 0) {
			let json = JSON.stringify(this.manifest, null, "\t");
			this.manifestPath = tmpPath + this.slash + "manifest.json";
			this.writeFileString(this.manifestPath, json);
			if (this.hasCreationMain)
				this.then("mcconfig", this.manifestPath, ...this.argv);
			else
				this.then("mcrun", this.manifestPath, ...this.argv);
		}
		else
			this.report(`### ${ this.errorCount } error(s)`);
	}
	
	
// URL

	getParentURL(href) {
		const url = new URL(href);
		let path = url.pathname;
		if (path) {
			let slash = path.lastIndexOf('/');
			url.pathname = path.slice(0, slash);
			return url.toString();
		}
	}
	isPathInvalid(path, index) {
		let split = path.split('/');
		if (index > 0)
			split = split.slice(index);
		return split.find(segment => {
			if (segment == "") return true;
			if (segment == ".") return true;
			if (segment == "..") return true;
			segment = decodeURIComponent(segment);
			segment = segment.toLowerCase();
			if (segment == "node_modules") return true;
			return false;
		}) ? true : false;
	}
	mergeURL(href, base) {
		const url = new URL(href, base);
		return url.toString();
	}
	pathToURL(path) {
		if (this.windows)
			path = "/" + path.replaceAll('\\', '/');
		return "file://" + path;
	}
	urlToDirectoryPath(href) {
		const url = new URL(href);
		if (url.protocol == "file:") {
			let path = url.pathname;
			if (this.windows)
				path = path.slice(1).replaceAll('/', '\\');
			if (this.isDirectoryOrFile(path) == -1)
				return path;
		}
	}
	urlToFilePath(href) {
		const url = new URL(href);
		if (url.protocol == "file:") {
			let path = url.pathname;
			if (this.windows)
				path = path.slice(1).replaceAll('/', '\\');
			if (this.isDirectoryOrFile(path) == 1)
				return path;
		}
	}
	validateURL(href) {
		try {
			const url = new URL(href);
			return url.toString();
		}
		catch {
		}
	}
	
// https://nodejs.org/api/esm.html#resolution-algorithm-specification
	
	throwInvalidModuleSpecifierError() {
		throw new Error("invalid module specifier");
	}
	throwInvalidPackageConfigurationError() {
		throw new Error("invalid package configuration");
	}
	throwInvalidPackageTargetError() {
		throw new Error("invalid package target");
	}
	throwModuleNotFoundError() {
		throw new Error("module not found");
	}
	throwPackageNotFoundError() {
		throw new Error("package not found");
	}
	throwPackagePathNotExportedError() {
		throw new Error("package path not exported!");
	}
	throwUnsupportedModuleFormat() {
		throw new Error("unsupported module format");
	}
	throwUnsupportedModuleSpecifier() {
		throw new Error("unsupported module specifier");
	}

	ESM_RESOLVE(specifier, parentURL) {
		let resolved = this.validateURL(specifier);
		if (!resolved) {
			if (specifier.startsWith('/') || specifier.startsWith('./') || specifier.startsWith('../'))
				resolved = this.mergeURL(specifier, parentURL);
			else if (specifier.startsWith('#'))
				resolved = this.PACKAGE_IMPORTS_RESOLVE(specifier, parentURL, defaultConditions);
			else
				resolved = this.PACKAGE_RESOLVE(specifier, parentURL);
		}
		if (resolved.startsWith("file://")) {
			if ((resolved.indexOf("%2F") >= 0) || (resolved.indexOf("%5C") >= 0))
				this.throwInvalidModuleSpecifierError();
			let path = this.urlToFilePath(resolved);
			if (!path)
				this.throwModuleNotFoundError();
			let format = this.ESM_FILE_FORMAT(resolved);
			if (format !== "module")
				this.throwUnsupportedModuleFormat();
		}
		return resolved;
	}
	PACKAGE_RESOLVE(packageSpecifier, parentURL) {
		let packageName;
		if (packageSpecifier === "")
			this.throwInvalidModuleSpecifierError();
//		If packageSpecifier is a Node.js builtin module name, then
//			Return the string "node:" concatenated with packageSpecifier.
		if (packageSpecifier.startsWith("embedded:"))
			return packageSpecifier;
		if (packageSpecifier.startsWith("moddable:"))
			return packageSpecifier;
		if (this.builtins.get("moddable:" + packageSpecifier))	
			return "moddable:" + packageSpecifier;
		if (!packageSpecifier.startsWith("@")) {
			let slash = packageSpecifier.indexOf('/');
			packageName = (slash >= 0) ? packageSpecifier.slice(0, slash) : packageSpecifier;
		}
		else {
			let slash = packageSpecifier.indexOf('/');
			if (slash < 0)
				this.throwInvalidModuleSpecifierError();
			slash = packageSpecifier.indexOf('/', slash + 1);
			packageName = (slash >= 0) ? packageSpecifier.slice(0, slash) : packageSpecifier;
		}
		if (packageName.startsWith(".") || (packageName.indexOf("\\") >= 0) || (packageName.indexOf("%") >= 0))
			this.throwInvalidModuleSpecifierError();
		let packageSubpath = '.' + packageSpecifier.slice(packageName.length);
		if (packageSubpath.endsWith("/"))
			this.throwInvalidModuleSpecifierError();
		let selfURL = this.PACKAGE_SELF_RESOLVE(packageName, packageSubpath, parentURL);
		if (selfURL !== undefined)
			return selfURL;
		while (parentURL != "file:///") {
			//@@ let packageURL = this.mergeURL("./node_modules/" + packageSpecifier, parentURL);
			let packageURL = this.mergeURL("./node_modules/" + packageName, parentURL);
			parentURL = this.getParentURL(parentURL);
			if (this.urlToDirectoryPath(packageURL)) {
				let pjson = this.READ_PACKAGE_JSON(packageURL);
				if (pjson !== null) {
					if ((pjson.exports !== null) & (pjson.exports !== undefined))
						return this.PACKAGE_EXPORTS_RESOLVE(packageURL, packageSubpath, pjson.exports, defaultConditions);
					if (packageSubpath == ".") {
						if (typeof pjson.main === "string")
							return this.mergeURL(pjson.main, packageURL + "/package.json");
					}
					else
						return this.mergeURL(packageSubpath, packageURL + "/package.json");
				}
			}
		}
		this.throwModuleNotFoundError();
	}
	PACKAGE_SELF_RESOLVE(packageName, packageSubpath, parentURL) {
		let packageURL = this.LOOKUP_PACKAGE_SCOPE(parentURL);
		if (packageURL === null)
			return undefined;
		let pjson = this.READ_PACKAGE_JSON(packageURL);
		if ((pjson === null) || (pjson.exports === null) || (pjson.exports === undefined))
			return undefined;
		if (pjson.name === packageName)
			return this.PACKAGE_EXPORTS_RESOLVE(packageURL, packageSubpath, pjson.exports, defaultConditions);
	}
	PACKAGE_EXPORTS_RESOLVE(packageURL, subpath, exports, conditions) {
		let dotKeys = 0;
		let noDotKeys = 0;
		if ((typeof exports === "object") && (!Array.isArray(exports))) {
			for (let key in exports) {
				if (key.startsWith("."))
					dotKeys++;
				else
					noDotKeys++;
			}
			if ((dotKeys > 0) && (noDotKeys > 0))
				this.throwInvalidPackageConfigurationError();
		}
		if (subpath === '.') {
			let mainExport;
			if (dotKeys == 0)
				mainExport = exports;
			else if ('.' in exports)
				mainExport = exports['.'];
			if (mainExport !== undefined) {
				let resolved = this.PACKAGE_TARGET_RESOLVE(packageURL, mainExport, null, false, conditions);
				if ((resolved !== undefined) && (resolved !== null))
					return resolved;
			}
		}
		else if (dotKeys > 0) {
			//@@ let matchKey = "./" + subpath;
			let matchKey = subpath;
			let resolved = this.PACKAGE_IMPORTS_EXPORTS_RESOLVE(matchKey, exports, packageURL, false, conditions);
			if ((resolved !== undefined) && (resolved !== null))
				return resolved;
		}
		this.throwPackagePathNotExportedError();
	}
	PACKAGE_IMPORTS_RESOLVE(specifier, parentURL, conditions) {
		if ((specifier === '#') || specifier.startsWith("#/"))
			this.throwInvalidModuleSpecifierError();
		let packageURL = this.LOOKUP_PACKAGE_SCOPE(parentURL);
		if (packageURL !== null) {
			let pjson = this.READ_PACKAGE_JSON(packageURL);
			if (pjson && pjson.imports && (typeof pjson.imports === "object")) {
				let resolved = this.PACKAGE_IMPORTS_EXPORTS_RESOLVE(specifier, pjson.imports, packageURL, true, conditions);
				if ((resolved !== undefined) && (resolved !== null))
					return resolved;
			}
		}
		this.throwInvalidModuleSpecifierError();
	}
	PACKAGE_IMPORTS_EXPORTS_RESOLVE(matchKey, matchObj, packageURL, isImports, conditions) {
		if ((matchKey in matchObj) && (matchKey.indexOf('*') < 0)) {
			let target = matchObj[matchKey];
			return this.PACKAGE_TARGET_RESOLVE(packageURL, target, null, isImports, conditions);
		}
		let expansionKeys = [];
		for (let key in matchObj) {
			let star = key.indexOf('*');
			if ((star >= 0) && (star == key.lastIndexOf('*')))
				expansionKeys.push(key);
		}
		expansionKeys.sort(this.PATTERN_KEY_COMPARE);
		for (let expansionKey of expansionKeys) {
			let star = expansionKey.indexOf('*');
			let patternBase = expansionKey.slice(0, star);
			if (matchKey.startsWith(patternBase) && (matchKey !== patternBase)) {
				let patternTrailer = expansionKey.slice(star + 1);
				if ((patternTrailer.length == 0) || (matchKey.endsWith(patternTrailer) && (matchKey.length >= expansionKey.length))) {
					let target = matchObj[expansionKey];
					let patternMatch = matchKey.slice(patternBase.length, matchKey.length - patternTrailer.length);
					return PACKAGE_TARGET_RESOLVE(packageURL, target, patternMatch, isImports, conditions);
				}
			}
		}
	}
	PATTERN_KEY_COMPARE(keyA, keyB) {
		let lengthA = keyA.length;
		let lengthB = keyB.length;
		let starA = keyA.indexOf('*');
		let starB = keyB.indexOf('*');
		let baseLengthA = (starA >= 0) ? starA + 1 : lengthA;
		let baseLengthB = (starB >= 0) ? starB + 1 : lengthB;
		if (baseLengthA > baseLengthB) return -1;
		if (baseLengthA < baseLengthB) return 1;
		if (starA < 0) return 1;
		if (starB < 0) return -1;
		if (lengthA > lengthB) return -1;
		if (lengthA < lengthB) return 1;
		return 0;
	}
	PACKAGE_TARGET_RESOLVE(packageURL, target, patternMatch, isImports, conditions) {
		if (target === null)
			return null;
		if (typeof target === "string") {
			if (!target.startsWith("./")) {
				if (!isImports || target.startsWith("../") || target.startsWith("/"))
					this.throwInvalidPackageTargetError();
				if (typeof patternMatch === "string")
					return this.PACKAGE_RESOLVE(target.replaceAll("*", patternMatch), packageURL + "/");
				return this.PACKAGE_RESOLVE(target, packageURL + "/");
			}
			if (this.isPathInvalid(target, 1))
				this.throwInvalidPackageTargetError();
			let resolvedTarget = this.mergeURL(target, packageURL + "/package.json");
			if (patternMatch == null)
				return resolvedTarget;
			if (this.isPathInvalid(patternMatch, 0))
				this.throwInvalidModuleSpecifierError();
			return resolvedTarget.replaceAll("*", patternMatch)
		}
		if (typeof target === "object") {
			if (Array.isArray(target)) {
				if (target.length === 0)
					return null;
				for (let targetValue of target) {
					let resolved, error;
					try {
						resolved = this.PACKAGE_TARGET_RESOLVE(packageURL, targetValue, patternMatch, isImports, conditions);
						if (resolved === undefined)
							continue;
						return resolved;
					}
					catch(e) {
						error = e;
					}
				}
				if (error)
					throw error;
				return undefined;
			}
			else {
				for (let p in target) {
					if ((p === "default") || (conditions.indexOf(p) >= 0)) {
						let targetValue = target[p];
						let resolved = this.PACKAGE_TARGET_RESOLVE(packageURL, targetValue, patternMatch, isImports, conditions);
						if (resolved === undefined)
							continue;
						return resolved;
					}
				}
				return undefined;
			}
		}
		this.throwInvalidPackageTargetError();
	}
	ESM_FILE_FORMAT(url) {
		if (url.endsWith(".mjs"))
			return "module";
		if (url.endsWith(".cjs"))
			return "commonjs";
		if (url.endsWith(".json"))
			return "json";
		let packageURL = this.LOOKUP_PACKAGE_SCOPE(url);
		if (packageURL) {
			let pjson = this.READ_PACKAGE_JSON(packageURL);
			if (pjson.type == "module") {
				if (url.endsWith(".js"))
					return "module";
			}
		}
	}
	LOOKUP_PACKAGE_SCOPE(url) {
		let scopeURL = url;
		while (scopeURL != "file:///") {
			scopeURL = this.getParentURL(scopeURL);
			if (scopeURL.endsWith("/node_modules"))
				return null;
			let pjsonURL = scopeURL + "/package.json";
			if (this.urlToFilePath(pjsonURL))
				return scopeURL;
		}
		return null;
	}
	READ_PACKAGE_JSON(packageURL) {
		let pjson = this.packages.get(packageURL);
		if (pjson !== undefined)
			return pjson;
		let path = this.urlToFilePath(packageURL + "/package.json");
		if (path)
			pjson = this.readFileJSON(path);
		else
			pjson = null;
		this.packages.set(packageURL, pjson);
		return pjson;
	}
	
// BUILT-INS

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
	filterManifestFile(path) {
		const parts = this.splitPath(path);
		if ("tls" === parts.name) parts.name = "manifest";		// workaround for tls.json which doesn't follow naming convention
		return parts.name.startsWith("manifest") && (parts.extension == ".json") && (parts.directory.indexOf("manifests") < 0);
	}
	getBuiltins(manifests) {
		const builtins = [];
		for (let manifest of manifests) {
			this.currentDirectory = manifest.directory;
			this.currentPath = manifest.path;
			const result = {};
			this.concatProperties(result, manifest.modules, true);
			if ("platforms" in manifest) {
				let platform = this.matchPlatform(manifest.platforms, this.fullplatform);
				if (platform)
					this.concatProperties(result, platform.modules, true);
			}
			var rule = new ModulesRule(this);
			this.jsFiles = [];
			this.jsFiles.already = {};
			this.jsFolders = [];
			this.jsFolders.already = {};
			this.tsFiles = [];
			this.tsFiles.already = {};
			this.dtsFiles = [];
			this.dtsFiles.already = {};
			rule.process(result);
			
			let path = manifest.from.slice(this.moddablePath.length);
			if (this.windows)
				path = path.replaceAll('\\', '/');
			for (let file of this.jsFiles) {
				let specifier = file.target.slice(0, -4);
				if (this.windows)
					specifier = specifier.replaceAll('\\', '/');
				let builtin = builtins.find(it => it.specifier == specifier);
				if (builtin)
					builtin.manifests.push(path);
				else
					builtins.push({ specifier, manifests:[path] });
			}
		}
		builtins.sort((a, b) => a.specifier.localeCompare(b.specifier));
		for (let builtin of builtins) {
			let specifier = builtin.specifier;
			if (specifier.startsWith("~.embedded/"))
				specifier = "embedded:" + specifier.slice(11);
			else
				specifier = "moddable:" + specifier;
			builtin.specifier = specifier
			builtin.manifests.sort();
		}
		return builtins;
	}
	getPlatformBuiltins() {
		const version = this.getToolsVersion();
		const platform = this.platform;
		let cachePath = this.createDirectories(this.outputPath, "tmp", platform);
		if (this.subplatform)
			cachePath = this.createDirectories(cachePath, this.subplatform);
		else if ((platform == "lin") || (platform == "mac") || (platform == "win"))
			cachePath = this.createDirectories(cachePath, "mc");
		cachePath += this.slash + "mcpack-cache.json";
// 		if (this.isDirectoryOrFile(cachePath) == 1) {
// 			try {
// 				const cacheBuffer = this.readFileString(cachePath);
// 				const cacheJSON = JSON.parse(cacheBuffer);
// 				if (cacheJSON.version == version) {
// 					return cacheJSON.builtins;
// 				}
// 			}
// 			catch {
// 			}
// 		}
		
		const currentDirectory = this.currentDirectory;
		this.manifests = [];
		this.manifests.already = {};
		const bases = [
			this.examplesPath + this.slash + "manifest_base.json",
			this.examplesPath + this.slash + "manifest_commodetto.json",
			this.examplesPath + this.slash + "manifest_piu.json",
			this.examplesPath + this.slash + "manifest_net.json",
			this.modulesPath + this.slash + "network" + this.slash + "ble" + this.slash + "manifest_client.json",
			this.modulesPath + this.slash + "network" + this.slash + "ble" + this.slash + "manifest_server.json",
			this.modulesPath + this.slash + "io" + this.slash + "manifest.json",
		];
		for (let base of bases)
			this.parseManifest(base, base);
		this.recurseDirectory(this.modulesPath, this.filterManifestFile, this.parseManifest);
		const builtins = this.getBuiltins(this.manifests);
		this.currentDirectory = currentDirectory;
		
		const cacheJSON = { version, builtins };
		const cacheBuffer = JSON.stringify(cacheJSON, null, "\t");
		this.writeFileString(cachePath, cacheBuffer);
		
		return builtins;
	}
	includeManifest(include, from, directory) {
		this.currentDirectory = directory;
		let path = this.resolveFilePath(this.resolveVariable(include));
		if (!path)
			throw new Error("'" + include + "': manifest not found!");
		this.parseManifest(path, from);
	}
	mapBuiltins(builtins) {
		const map = new Map;
		builtins.forEach(builtin => {
			map.set(builtin.specifier, builtin);
		});
		return map;
	}
	matchPlatform(platforms, name) {
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
		for (let n in platforms) {
			if ("..." === n)
				return platforms[n];
		}
	}
	parseBuild(platform) {
		let properties = platform.build;
		if (properties) {
			for (let name in properties) {
				let value = properties[name];
				if (typeof value == "string") {
					const dotSlash = "." + this.slash;
					value = this.resolveVariable(value);
					if (value.startsWith(dotSlash)) {
						const path = this.resolveDirectoryPath(dotSlash);
						if (path) {
							if (dotSlash == value)
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
	parseManifest(path, from) {
		if (this.manifests.already[path])
			return;
		const parts = this.splitPath(path);
		this.currentDirectory = parts.directory;
		this.currentPath = path;
		const manifest = this.readFileJSON(path);
		manifest.directory = parts.directory;
		manifest.path = path;
		manifest.from = from ?? path;
		this.manifests.already[path] = manifest;
		
		this.parseBuild(manifest);
		if (manifest.error) {
			if ("string" === typeof manifest.error)
				manifest.error = [ manifest.error ];
		}
		else
			manifest.error = [];
		if (manifest.include) {
			if ("string" === typeof manifest.include)
				manifest.include = [ manifest.include ];
		}
		else
			manifest.include = [];
		if ("platforms" in manifest) {
			let platforms = manifest.platforms;
			let platform = this.matchPlatform(platforms, this.fullplatform);
			if (platform) {
				this.parseBuild(platform);
				if (platform.error)
					manifest.error = manifest.error.concat(platform.error);
				if (platform.include)
					manifest.include = manifest.include.concat(platform.include);
			}
		}
		if (manifest.error.length == 0) {
			manifest.include.forEach(include => this.includeManifest(include, from, manifest.directory));
			this.manifests.push(manifest);
		}
	}
	recurseDirectory(folderPath, filter, callback) {
		const names = this.enumerateDirectory(folderPath);
		for (let name of names) {
			const path = folderPath + this.slash + name;
			if (this.isDirectoryOrFile(path) < 0) {
				this.recurseDirectory(path, filter, callback);
			}
			else {
				if (filter) {
					if (filter.call(this, path))
						callback.call(this, path);
				}
				else
					callback.call(this, path);
			}
		}
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
		try {
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
		catch(e) {
			this.reportError(this.currentPath, 1, e.message);
		}
		return null;
	}
	resolveVariable(value) {
		value = value.replace(/\$\(([^\)]+)\)/g, (offset, value) => {
			if (value in this.environment)
				return this.environment[value];
			return this.getenv(value);
		});
		return this.resolveSlash(value);
	}
	selectManifest(builtin) {
		let index;
		if (builtin.manifests.length == 1)
			index = 0;
		else {
			const parts = builtin.manifests.map(it => it.split(this.slash));
			index = parts.findIndex(part => part.indexOf("manifest_base.json") >= 0);
			if (index < 0) {
				index = parts.findIndex(part => part.indexOf(builtin.specifier) >= 0);
			}
		}
		return index;
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
		var sourceParts = tool.splitPath(source);
		var slash = source.lastIndexOf(tool.slash);
		var directory = this.tool.resolveDirectoryPath(sourceParts.directory);
		if (directory) {
			this.count = 0;
			var star = sourceParts.name.lastIndexOf("*");
			var prefix = (star >= 0) ? sourceParts.name.slice(0, star) : sourceParts.name;
			var names = tool.enumerateDirectory(directory);
			var c = names.length;
			for (var i = 0; i < c; i++) {
				var name = names[i];
				if (name[0] == ".")
					continue;
				var path = directory + tool.slash + name;
				var parts = tool.splitPath(path);
				if ((".ts" === parts.extension) && parts.name.endsWith(".d")) {
					parts.name = parts.name.slice(0, parts.name.length - 2);
					parts.extension = ".d.ts";
				} 
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
					if ((parts.name == prefix) && ((sourceParts.extension == "") || (sourceParts.extension == parts.extension)))
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
							if (source) {
								if (typeof source == "string")
									source = tool.resolveSlash(source);
								else
									source.source = tool.resolveSlash(source.source);
								this.iterate(target, source, true, suffix, false);
							}
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

class ModulesRule extends Rule {
	appendSource(target, source, include, suffix, parts, kind, query) {
		var tool = this.tool;
		if (kind < 0)
			return;
		if ((parts.extension == ".js") || (parts.extension == ".mjs"))
			this.appendFile(tool.jsFiles, target + ".xsb", source, include);
		else if (parts.extension == ".ts")
			this.appendFile(tool.tsFiles, target + ".xsb", source, include);
		else if (parts.extension == ".d.ts")
			this.appendFile(tool.dtsFiles, target, source, include);
	}
	appendTarget(target) {
		this.appendFolder(this.tool.jsFolders, target);
	}
	noFilesMatch(source, star) {
// 		this.tool.reportWarning(null, 0, "no modules match: " + source);
	}
};

