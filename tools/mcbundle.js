import {TOOL, FILE} from "tool";

class ShellFile extends FILE {
	constructor(path) {
		super(path)
	}
}

const devices = [
	{ platform:"esp/moddable_one", id:"com.moddable.one", targets:[ "main.bin" ] },
	{ platform:"esp/moddable_display_1", id:"com.moddable.display_1", targets:[ "main.bin" ] },
	{ platform:"esp/moddable_three", id:"com.moddable.three", targets:[ "main.bin" ] },
	{ platform:"esp/moddable_display_3", id:"com.moddable.display_3", targets:[ "main.bin" ] },
	{ platform:"esp/nodemcu", id:"com.nodemcu.esp8266", targets:[ "main.bin" ] },
	{ platform:"esp32/esp32_thing", id:"com.sparkfun.thing.esp32", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/esp32_thing_plus", id:"com.sparkfun.thing_plus.esp32", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/kaluga", id:"com.espressif.kaluga", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5atom_echo", id:"com.m5atom.echo", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5atom_lite", id:"com.m5atom.lite", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5atom_matrix", id:"com.m5atom.matrix", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5atom_u", id:"com.m5atom.u", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5stack_core2", id:"com.m5stack.core2", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5stack_fire", id:"com.m5stack.fire", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5stack", id:"com.m5stack", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5stick_c", id:"com.m5stick.c", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/m5stick_cplus", id:"com.m5stick.cplus", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/moddable_two", id:"com.moddable.two", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/moddable_display_2", id:"com.moddable.display_2", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/nodemcu", id:"com.nodemcu.esp32", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/saola_wroom", id:"com.espressif.saola", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"esp32/wrover_kit", id:"com.esp32.wrover", targets:[ "bootloader.bin", "partition-table.bin", "xs_esp32.bin" ] },
	{ platform:"wasm", id:"com.moddable.wasm", targets:[ "mc.js", "mc.wasm" ] },
];

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.moddablePath = this.getenv("MODDABLE");
		if (!this.moddablePath)
			throw new Error("MODDABLE: variable not found!");
		this.debug = false;
		this.environment = { "MODDABLE": this.moddablePath }
		this.make = false;
		this.manifestPath = null;
		this.outputPath = null;
		this.temporary = false;
		this.verbose = false;
		this.windows = this.currentPlatform == "win";
		this.slash = this.windows ? "\\" : "/";
		const argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {
			case "-d":
				this.debug = true;
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
			default:
				name = argv[argi];
				if (this.manifestPath)
					throw new Error("'" + name + "': too many manifests!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': manifest not found!");
				this.manifestPath = path;
				break;
			}
		}
		if (this.manifestPath) {
			const parts = this.splitPath(this.manifestPath);
			this.currentDirectory = parts.directory;
		}
		else {
			const path = this.resolveFilePath("." + this.slash + "manifest.json");
			if (path)
				this.manifestPath = path;
			else
				throw new Error("no manifest!");
		}
		if (!this.outputPath)
			this.outputPath = this.currentDirectory;
			
		var parts = this.splitPath(this.currentDirectory);
		this.environment.NAME = parts.name + parts.extension;
	}
	parseBuild(platform) {
		let properties = platform.build;
		if (properties) {
			for (let name in properties) {
				let value = properties[name];
				if (typeof value == "string")
					this.environment[name] = this.resolveVariable(value);
				else
					this.environment[name] = value;
			}
		}
	}
	parseManifest(path) {
		const buffer = this.readFileString(path);
		try {
			return JSON.parse(buffer);
		}
		catch (e) {
			const message = e.toString();
			const result = /SyntaxError: ([^:]+: )?([0-9]+): (.+)/.exec(message);
			if (result.length == 4) {
				this.reportError(path, parseInt(result[2]), result[3]);
			}
			throw new Error("'" + path + "': invalid manifest!");;
		}
	}
	resolveSlash(value) {
		if (this.windows)
			value = value.replace(/\//g, "\\");
		return value;
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
		const path = this.manifestPath;
		const manifest = this.parseManifest(path);
		this.parseBuild(manifest);
		const store = manifest.bundle ?? manifest.store;
		if (!store) {
			throw new Error("no store!");;
		}
		
		const id = store.id;
		if (!id)
			throw new Error("no id!");;
		const parts = id.split(".");
		if (parts.length != 3)
			throw new Error("invalid id!");;
		const signature = parts.reverse().join(".");
		
		const names = store.devices;
		if (!names)
			throw new Error(`no devices!`);
		const results = [];
		for (let name of names) {
			let gotDevice = false;
			let star = name.lastIndexOf("*");
			if (star >= 0) {
				star = name.slice(0, star) 
				for (let device of devices) {
					if (device.id.indexOf(star) == 0) {
						gotDevice = true;
						if (results.indexOf(device) < 0)
							results.push(device);
					}
					else if (device.platform.indexOf(star) == 0) {
						gotDevice = true;
						if (results.indexOf(device) < 0)
							results.push(device);
					}
				}
			}
			else {
				for (let device of devices) {
					if (device.id == name) {
						gotDevice = true;
						if (results.indexOf(device) < 0)
							results.push(device);
						break;
					}
					if (device.platform == name) {
						gotDevice = true;
						if (results.indexOf(device) < 0)
							results.push(device);
						break;
					}
				}
			}
			if (!gotDevice)
				throw new Error(`device not found: ${name}!`);
		}
		if (results.length == 0)
			throw new Error(`no devices!`);
			
		let icon = store.icon;
		if (!icon)
			icon = "./icon.png";
		let iconPath = this.resolveFilePath(icon);
		if (!iconPath)
			this.reportWarning(null, 0, `icon not found: ${icon}!`);
		
		let custom = store.custom;
		if (!custom)
			custom = "./custom";
		let customPath = this.resolveDirectoryPath(custom);
		if (customPath) {
			const directoryNames = this.enumerateDirectory(customPath);
			for (let directoryName of directoryNames) {
				const directory = customPath + "/" + directoryName;
				if (this.isDirectoryOrFile(directory) < 0) {
					const fileNames = this.enumerateDirectory(directory);
					let gotIcon = false;
					let gotIndex = false;
					for (let fileName of fileNames) {
						if (fileName == "icon.png")
							gotIcon = true;
						else if (fileName == "index.html")
							gotIndex = true;
						else if (fileName.indexOf('.') != 0)
							throw new Error(`unexpected file: ${custom}/${directoryName}/${fileName}!`);
					}
					if (!gotIcon)
						this.reportWarning(null, 0, `icon not found: ${custom}/${directoryName}/icon.png!`);
					if (!gotIndex)
						throw new Error(`missing file: ${custom}/${directoryName}/index.html!`);
				}
				else if (directoryName.indexOf('.') != 0)
					throw new Error(`unexpected file: ${custom}/${directoryName}!`);
			}
		}
		const outputPath = this.outputPath;
		let sourceESP32 = false;
		let sourceWASM = false;
		for (let result of results) {
			if (result.platform.indexOf("esp32") == 0)
				sourceESP32 = true;
			if (result.platform.indexOf("wasm") == 0)
				sourceWASM = true;
		}
		const build = this.debug ? "debug" : "release";
		const name = this.environment.NAME;
		const option = this.debug ? "-d" : "";
		const filePath = (this.windows) ? `${outputPath}\\${id}.bat` : `${outputPath}/${id}.sh`;
		const file = new ShellFile(filePath)
		if (!this.windows) {
			file.line("#!/bin/bash");
			file.line(`OUTPUT=${outputPath}/${id}`);
			file.line(`rm -R $OUTPUT`);
			file.line(`mkdir $OUTPUT`);
			if (sourceESP32)
				file.line(`source $IDF_PATH/export.sh`);
			if (sourceWASM)
				file.line(`source $EMSDK/emsdk_env.sh`);
			for (let result of results) {
				file.line(`echo "# ${result.id}"`);
				file.line(`mcconfig ${option} -m -p ${result.platform} -s ${signature} -t build`);
				file.line(`mkdir $OUTPUT/${result.id}`);
				for (let target of result.targets) {
					file.line(`cp $MODDABLE/build/bin/${result.platform}/${build}/${name}/${target} $OUTPUT/${result.id}/${target}`);
				}
			}
			if (iconPath)
				file.line(`cp ${iconPath} $OUTPUT/icon.png`);
			if (customPath) {
				file.line(`cp -R ${customPath} $OUTPUT`);
			}
			file.line(`rm -f $OUTPUT.zip`);
			file.line(`cd ${outputPath}`);
			file.line(`zip -r ${id}.zip ${id}`);
			file.close();
			if (this.make)
				this.then("bash", filePath);
		}
		else {
			file.line(`set OUTPUT=${outputPath}\\${id}`);
			file.line(`rmdir /s /q %OUTPUT%`);
			file.line(`mkdir %OUTPUT%`);
			if (sourceESP32)
				file.line(`call %IDF_PATH%\\export.bat`);
			if (sourceWASM)
				throw new Error("WASM mcbundle not yet supported on Windows");
			for (let result of results) {
				file.line(`echo "# ${result.id}"`);
				file.line(`call mcconfig ${option} -m -p ${result.platform} -s ${signature} -t build`);
				file.line(`mkdir %OUTPUT%\\${result.id}`);
				for (let target of result.targets) {
					file.line(`copy %MODDABLE%\\build\\bin\\${this.resolveSlash(result.platform)}\\${build}\\${name}\\${target} %OUTPUT%\\${result.id}\\${target}`);
				}
			}
			if (iconPath)
				file.line(`copy ${iconPath} %OUTPUT%\\icon.png`);
			if (customPath)
				file.line(`xcopy ${customPath} %OUTPUT%\\custom /E/H/I`);
				
			file.line(`del /f/q %OUTPUT%.zip`);
			file.line(`cd ${outputPath}`);
			file.line(`tar.exe -a -c -f ${id}.zip ${id}`);
			file.close();
			if (this.make)
				this.then(filePath);
		}
	}
}
