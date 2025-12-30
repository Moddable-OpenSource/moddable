/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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
					throw new Error("-o '" + name + "': too many output directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': directory not found!");
				this.outputDirectory = path;
				break;

			case "-p":
				argi++;	
				if (argi >= argc)
					throw new Error("-p no platform!");
				if ("zephyr" !== argv[argi])
					throw new Error("Zephyr RTOS output only");
				break;

			case "-c":
				if (++argi >= argc)
					throw new Error("-c no zephyr.conf!");
				path = this.resolveFilePath(argv[argi]);
				this.zephyrConfig ??= new Map;
				this.readFileString(path).split("\n").map(line => line.trim()).forEach(line => {
					const parts = line.split("=");
					if (2 !== parts.length) return;
					if (!parts[0].startsWith("CONFIG_")) return;
					this.zephyrConfig.set(parts[0], parts[1]);
				});
				break;

			case "-d":
				if (++argi >= argc)
					throw new Error("-d no mc.defines.h!");
				path = this.resolveFilePath(argv[argi]);
				this.defines ??= new Map;
				this.readFileString(path).split("\n").map(line => line.trim()).forEach(line => {
					const match = line.match(/^\s*#define\s+(\S+)\s+(.+?)\s*$/);
					if (!match) return;

					let value = match[2];
					if (value.startsWith('(') && value.endsWith(')'))
						value = value.slice(1, -1);

					if (value.startsWith('"') && value.endsWith('"'))
						value = value.slice(1, -1);
					else if (/^-?\d+$/.test(value))
						value = parseInt(value, 10);

					this.defines.set(match[1], value);
				});
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

		if (!this.outputDirectory)
			throw new Error("no output direcetory!");

		if (!this.zephyrConfig)
			throw new Error("no zephyr.conf!");

		if (!this.defines)
			throw new Error("no mc.defines.h!");
 	}

	run() {
		let dts = String.fromArrayBuffer(this.readFileBuffer(this.sourcePath));
		dts = dts.replace(/\/\*[\s\S]*?\*\//g, '') // Remove block comments
				.replace(/[ \t]+/g, ' ')           // Normalize whitespace
				.replace(/\n\s*\n\s*/g, '\n');     // Remove extra blank lines

		const parser = new DTSParser();
		const parsed = parser.parse(dts);

    const compatible = new Map;
    const bindingDirectory = this.resolveDirectoryPath(this.getenv("ZEPHYR_BASE") + "/dts/bindings");
    for (const kind of this.enumerateDirectory(bindingDirectory)) {
        if ("." === kind.startsWith("."))
            continue;
        if (this.isDirectoryOrFile(bindingDirectory + "/" + kind) >= 0)
            continue;
        compatible.set(kind, this.enumerateDirectory(bindingDirectory + "/" + kind).filter(name => {
            if (name.startsWith("."))
              return;
            if (!name.includes(","))
              return;

            if ("st,dsi-lcd-qsh-030.yaml" === name)   //@@ hack so stm32u5a9j_dk links
              return;
            if (("nordic,nrf-gpio-forwarder.yaml" === name) || ("nordic,nrf-gpiote.yaml" === name))   //@@ hack so raytac_mdbt53 links
              return;
            if ("arduino,uno-adc.yaml" === name)   //@@ hack so nrf52840dk links
              return;
            if (("raspberrypi,pico-gpio.yaml" === name) || ("raspberrypi,pico-header.yaml" === name))  //@@ hack so pico_plus2 links
              return;

            return name.endsWith(".yaml");
        }).map(name => name.substring(0, name.length - 5)));
    }

		const state = {
			cCode: "",
			hCode: "",
			jsCode: "",
			tsCode: "",
			aliasTable: new Map,
			zephyrConfig: this.zephyrConfig,
			defines: this.defines,
      compatible
		}

		state.hCode +=
`#ifndef __MC_ZEPHYR_H__
#define __MC_ZEPHYR_H__

`;

		state.cCode +=
`#include "./mc.devicetree.h"
#include <zephyr/device.h>
#include "xsHost.h"
#include "mc.defines.h"

`;

		state.jsCode += `
const device = {
	io: {},
	pin: {}
};
`
		state.tsCode += `
declare module "embedded:provider/builtin" {
  interface DeviceIO {}
  interface DeviceNetwork {}

  interface Device {
    io: DeviceIO;
    network: DeviceNetwork;
  }
}

`;

		doAliases(state, parsed);
		doGPIOBanks(state, parsed); 
		doGPIOs(state, parsed);

		doBus(state, parsed, {
			binding: "i2c",
			name: "I2C",
      hostProviderName: "i2c",
			header: "#include <zephyr/drivers/i2c.h>",
      moduleSpecifier: "embedded:io/i2c",
      moduleDefault: "I2C",
			static:
`import I2C from "embedded:io/i2c";
device.io.I2C = I2C;
// import SMBus from "embedded:io/smbus";
// device.io.SMBus = SMBus;

device.i2c = {};
`
		});

		doBus(state, parsed, {
			binding: "serial",
			name: "Serial",
      hostProviderName: "serial",
			header: "#include <zephyr/drivers/uart.h>",
      moduleSpecifier: "embedded:io/serial",
      moduleDefault: "Serial",
			static:
`import Serial from "embedded:io/serial";
device.io.Serial = Serial;

device.serial = {};
`
		});

    const user = parsed.nodes['/'].children["zephyr,user"];
    const hasIOChannels = undefined !== user?.properties["io-channels"];

		if (("y" === state.zephyrConfig.get("CONFIG_ADC")) && hasIOChannels) {
			doBus(state, parsed, {
				binding: "adc",
				name: "Analog",
        hostProviderName: "analog",
				header: "#include <zephyr/drivers/adc.h>",
        moduleSpecifier: "embedded:io/analog",
        moduleDefault: "Analog",
				static:
`import Analog from "embedded:io/analog";
device.io.Analog = Analog;

device.analog = {};
`
			});
		}

		doBus(state, parsed, {
			binding: "pwm",
			name: "PWM",
      hostProviderName: "pwm",
			header: "#include <zephyr/drivers/pwm.h>",
      moduleSpecifier: "embedded:io/pwm",
      moduleDefault: "PWM",
			static:
`import PWM from "embedded:io/pwm";
device.io.PWM = PWM;

device.pwm = {};
`
		});

    if ("y" === state.zephyrConfig.get("CONFIG_RTC")) {
      let rtcs = doBus(state, parsed, {
          binding: "rtc",
          name: "RTC",
          header: "#include <zephyr/drivers/rtc.h>",
          js: false,
        });

  
      if (rtcs?.length) {
        state.jsCode += `
import RTC from "embedded:RTC/zephyr-builtin";
device.rtc = Object.freeze({io: RTC, port: "${rtcs[0].label}"});
`;
        state.tsCode += `
declare module "embedded:provider/builtin" {
	import RTC from "embedded:RTC"

	type RTCOptions = ConstructorParameters<typeof RTC>[0] & {
		io: typeof RTC
	}

	interface Device {
		rtc: {
			default: RTCOptions;
		}
	}
}
`;
      }
    }

    if ("y" === state.zephyrConfig.get("CONFIG_DISPLAY")) {
      doBus(state, parsed, {
        binding: "display",
        name: "Display",
        hostProviderName: "display",
        header: "#include <zephyr/drivers/display.h>",
        moduleSpecifier: "embedded:display",
        moduleDefault: "Display",
        tsDeviceIO: false,
        static:
`import Display from "embedded:display/zephyr";
device.display = {};
`
      });
    }

    doNetworkInterfaces(state, parsed);

/*
doBus(state, parsed, {
			binding: "spi",
			name: "SPI",
      hostProviderName: "spi",
			header: "#include <zephyr/drivers/spi.h>",
      moduleSpecifier: "embedded:io/spi",
      moduleDefault: "SPI",
			static:
`import SPI from "embedded:io/spi";
device.io.SPI = SPI;

device.spi = {};
`
		});
*/


    if ("y" === state.zephyrConfig.get("CONFIG_FILE_SYSTEM"))
      doFileSystems(state, parsed);

    if (("y" === state.zephyrConfig.get("CONFIG_FLASH")) && state.defines.get("MODDEF_ECMA419_FLASH"))
      doFlash(state, parsed);

    if ("y" === state.zephyrConfig.get("CONFIG_SETTINGS"))
      doKeyValue(state, parsed);

      state.hCode +=
`
#endif /* __MC_ZEPHYR_H__ */
`;

		state.jsCode += `
export default device;
`;

		state.tsCode += `
declare module "embedded:provider/builtin" {
  const device: Device
  export default device;
}
`;

const parts = {
				name: "mc.devicetree",
				directory: this.outputDirectory,
		};

		["h", "c", "js", "ts"].forEach(extension => {
      const value = state[extension + "Code"];
      if ("ts" === extension) extension = "d.ts";
			parts.extension = "." + extension;
			const output = new FILE(this.joinPath(parts), "wb");
			output.writeString(value);
			output.close();
		});
	}
}

function getNodes(state, dts, kind) {
  const compatibles = state.compatible.get(kind);
  if (!compatibles)
    return [];

  return DTSParser.filter(dts, node => {
      const compatible = node.properties?.compatible;
      if (!compatible)
          return;

      const status = node.properties.status?.value?.value ?? "okay"
      if ("okay" !== status)
        return;

      if ("string" === compatible.value.type)
        return compatibles.includes(compatible.value.value);

      if ("string-array" === compatible.value.type)
        return compatible.value.value.some(value => compatibles.includes(value));
    });
}

function doAliases(state, dts) {
	const root = dts.nodes['/'];
	const aliases = root.children.aliases;
	if (!aliases?.properties)
		return;

	for (let alias in aliases.properties) {
		const target = aliases.properties[alias].value.value;
		const targets = state.aliasTable.get(target);

		alias = alias.replaceAll('-', '_');
		if (targets)
			targets.push(alias);
		else
			state.aliasTable.set(target, [alias]);
	}
}

function doGPIOBanks(state, dts) {
	const gpios = getNodes(state, dts, "gpio");

	state.hCode += `
#define kModZephyrGPIOBankCount (${gpios.length})
`;

	if (0 === gpios.length)
		return;

	state.hCode += `
#include <zephyr/drivers/gpio.h>

struct modZephyrGPIOBank {
	const char *label;
	const struct device *device;
	uint8_t bankIndex;
	uint8_t gpioCount;
};

extern const struct modZephyrGPIOBank *modZephyrGetGPIOBank(const char *label);

`;

	state.gpioBanks = [];
	state.cCode +=`
static const struct modZephyrGPIOBank gGPIOBank[] = {
`;

	gpios.forEach((gpio, bankIndex) => {
		const ngpios = gpio.properties?.ngpios?.value.value[0];
		state.cCode += `	{
		.label = "${gpio.label}",
		.device = DEVICE_DT_GET(DT_NODELABEL(${gpio.label})),
		.bankIndex = ${bankIndex},
		.gpioCount = ${ngpios ? parseInt(ngpios) : "GPIO_MAX_PINS_PER_PORT"}
	},
`;
		state.gpioBanks.push(gpio.label);
	});

	state.cCode += `};

const struct modZephyrGPIOBank *modZephyrGetGPIOBank(const char *label)
{
	for (int i = 0; i < ARRAY_SIZE(gGPIOBank); i++) {
		if (0 == c_strcmp(gGPIOBank[i].label, label))
			return &gGPIOBank[i];
	}

	return C_NULL;
}
`;

	state.jsCode += `
import Digital from "embedded:io/digital";
device.io.Digital = Digital;

import DigitalBank from "embedded:io/digitalbank";
device.io.DigitalBank = DigitalBank;

`;
}

function doGPIOs(state, dts) {
	if (!state.gpioBanks?.length)
			return;

	const root = dts.nodes['/'];
	const gpios = [];
	["gpio-leds", "gpio-keys"].forEach(kind => {
		for (let what in root.children) {
			const node = root.children[what];
			if (kind !== node.properties.compatible?.value.value)
					continue;

			for (let item in node.children) {
				item = node.children[item];
				const g = item.properties.gpios.value.value; 
				const bus = g[0].slice(1);
				const labels = item.labels ?? (item.label ? [item.label] : []);
				gpios.push({
					kind,
					name: item.name,
					labels,
					userName: item.properties?.label?.value?.value,
					bankIndex: state.gpioBanks.indexOf(bus),
					bus,
					pin: parseInt(g[1]),
					flags: parseInt(g[2])
				});
			}
			return;
		}
	});

	if (0 === gpios.length)
		return;

  state.tsCode += `
declare module "embedded:provider/builtin" {
  import Digital from "embedded:io/digital";
  import DigitalBank from "embedded:io/digitalbank";
  
  interface DeviceIO {
    Digital: typeof Digital;
    DigitalBank: typeof DigitalBank;
  }
`;

	const kinds = new Set, leds = new Set, buttons = new Set;
	gpios.forEach(gpio => {
		let mode = [], kindName, edge;
		if ("gpio-leds" == gpio.kind) {
			mode.push("Digital.Output");
			kindName = "led";
			//@@ flags OutputOpenDrain
			if (gpio.flags & (1 << 0))		// GPIO_ACTIVE_LOW
				mode.push("Digital.ActiveLow");
      leds.add(gpio.name);
		}
		else if ("gpio-keys" === gpio.kind) {
			mode.push("Digital.Input");
			kindName = "button";
			if (gpio.flags & (1 << 5))		// GPIO_PULL_DOWN
				mode.push("Digital.InputPullDown");
			if (gpio.flags & (1 << 4))		// GPIO_PULL_UP
				mode.push("Digital.InputPullUp");
			if (gpio.flags & (1 << 0))		// GPIO_ACTIVE_LOW
				mode.push("Digital.ActiveLow");
			edge = "Digital.Rising | Digital.Falling";
      buttons.add(gpio.name);
		}
		mode = mode.join(" | ");

		if (!kinds.has(gpio.kind)) {
			state.jsCode += `device.${kindName} = {};\n`
			kinds.add(gpio.kind);
		}

		state.jsCode += `
device.${kindName}.${gpio.name} = class {${gpio.userName ? " // " + gpio.userName : ""}
	constructor (options) {
		return new Digital({
			${edge ? "edge: " + edge + ",\n			" : ""}...options,
			pin: {port: "${gpio.bus}", pin: ${gpio.pin}},
			mode: ${mode},
		});
	}
};
`;

		for (let i = 0; i < gpio.labels.length; i++) {
			const target = gpio.labels[i];
			const aliasTable = state.aliasTable.get(target);
			aliasTable?.forEach(alias => {
				state.jsCode += `device.${kindName}.${alias} = device.${kindName}.${gpio.name};\n`;
       	if ("led" === kindName)
      		leds.add(alias);
      	else
      		buttons.add(alias);

				if (("sw0" === alias) && ("gpio-keys" === gpio.kind))
					state.jsCode += `device.pin.button = Object.freeze({port: "${gpio.bus}", pin: ${gpio.pin}});\n`;
				if (("led0" === alias) && ("gpio-leds" === gpio.kind))
					state.jsCode += `device.pin.led = Object.freeze({port: "${gpio.bus}", pin: ${gpio.pin}});\n`;
			});
		}
	});


if (leds.size) {
		state.tsCode += `
  interface DeviceLEDOptions extends Omit<ConstructorParameters<typeof Digital>[0], 'pin' | 'mode' | 'port'> {}
  interface DeviceLEDs {
`;
    leds.forEach(value => {
      state.tsCode += `    ${value}: new (options?: DeviceLEDOptions) => InstanceType<typeof Digital>\n`;
    });
    state.tsCode +=`  }\n`;
	}

	if (buttons.size) {
		state.tsCode += `
  interface DeviceButtonOptions extends Omit<ConstructorParameters<typeof Digital>[0], 'pin' | 'mode' | 'port' > {
      onReadable?: (this: Digital) => void;
  }
  interface DeviceButtons {
`;
    buttons.forEach(value => {
      state.tsCode += `    ${value}: new (options?: DeviceButtonOptions) => InstanceType<typeof Digital>\n`;
    });
    state.tsCode +=`  }\n`;
	}

  if (leds.size || buttons.size) {
		state.tsCode += `\n  interface Device {\n`;
    if (leds.size)
  		state.tsCode += `    leds: DeviceLEDs\n`;
    if (buttons.size)
  		state.tsCode += `    buttons: DeviceButtons\n`;
		state.tsCode += `  }\n`;
  }

  state.tsCode +=
`}
`;
}

function doBus(state, dts, options) {
  const nodes = getNodes(state, dts, options.binding);

	state.hCode += `
#define kModZephyr${options.name}BusCount (${nodes.length})
`;

	if (0 === nodes.length)
		return;

	let busSpecific = "";
	if ("spi" === options.binding)
			busSpecific = "\n	struct gpio_dt_spec cs;"

	state.hCode += `
${options.header}

struct modZephyr${options.name} {
	const char *label;
	const struct device *device;
	uint8_t busIndex;${busSpecific}
};

extern const struct modZephyr${options.name} *modZephyrGet${options.name}(const char *label);

`;

	state.cCode +=`
static const struct modZephyr${options.name} g${options.name}[] = {
`;

	nodes.forEach((node, index) => {
		busSpecific = "";
		if ("spi" === options.binding) {
				const cs = node.properties["cs-gpios"]?.value?.value;
				if (cs) {
					busSpecific = `\n		.cs.port = DEVICE_DT_GET(DT_NODELABEL(${cs[0].slice(1)})),
		.cs.pin = ${parseInt(cs[1])},
		.cs.dt_flags = ${parseInt(cs[2])},`; 
				}
		}


		state.cCode += `	{
		.label = "${node.label}",
		.device = DEVICE_DT_GET(DT_NODELABEL(${node.label})),
		.busIndex = ${index},${busSpecific}
	},
`;
	});

	state.cCode += `};

const struct modZephyr${options.name} *modZephyrGet${options.name}(const char *label)
{
	for (int i = 0; i < ARRAY_SIZE(g${options.name}); i++) {
		if (0 == c_strcmp(g${options.name}[i].label, label))
			return &g${options.name}[i];
	}

	return C_NULL;
}
`;

  if (false === options.js)
      return nodes;

  state.jsCode += "\n" + options.static;
  const hostProviderName = options.hostProviderName ?? options.name;

  nodes.forEach(node => {
		let additional = "";
		if ("serial" === options.binding) {
			const baud = node.properties["current-speed"]?.value?.value?.[0];
			if (baud)
				additional += `baud: ${parseInt(baud)}`;
		}
		state.jsCode += `device.${hostProviderName}.${node.label} = Object.freeze({io: ${options.name}, port: "${node.label}"${additional ? ", " + additional : ""}});\n`;
		for (let i = 1; i < node.labels?.length; i++) 
			state.jsCode += `device.${hostProviderName}.${node.labels[i]} = device.${hostProviderName}.${node.label};\n`;
	});

  // use first one as default - just a guess. always correct when there is just one bus. is there a "chosen" for this??
  let defaultLabel = nodes[0].label;
	const chosen = dts.nodes['/'].children.chosen;

  if ("display" === options.binding) {
    const t = chosen?.properties["zephyr,display"];
    if ("reference" === t?.value.type)
        defaultLabel = t.value.value;
  }

	state.jsCode += `device.${hostProviderName}.default = device.${hostProviderName}.${defaultLabel};\n`;

  if (!options.moduleDefault || !options.moduleSpecifier)
    return nodes;   //@@

  state.tsCode += `declare module "embedded:provider/builtin" {\n`;
  state.tsCode += `\timport ${options.moduleDefault} from "${options.moduleSpecifier}"\n`;
  state.tsCode += "\n";
  if (options.tsDeviceIO ?? true) {
    state.tsCode += "\tinterface DeviceIO {\n";
    state.tsCode += `\t\t${options.moduleDefault}: typeof ${options.moduleDefault}\n`;
    state.tsCode += "\t}\n";
    state.tsCode += "\n";
  }
  state.tsCode += `\ttype ${options.moduleDefault}Options = ConstructorParameters<typeof ${options.moduleDefault}>[0] & {\n`;
  state.tsCode += `\t\tio: typeof ${options.moduleDefault}\n`;
  state.tsCode += "\t}\n";
  state.tsCode += "\n";
  state.tsCode += "\tinterface Device {\n";
  state.tsCode += `\t\t${hostProviderName}: {\n`;
  const tsType = `${options.moduleDefault}Options`;
  state.tsCode += `\t\t\tdefault: ${tsType};\n`;
  nodes.forEach(node => {
		state.tsCode += `\t\t\t${node.label}: ${tsType};\n`;
		for (let i = 1; i < node.labels?.length; i++) 
      state.tsCode += `\t\t\t${node.labels[i]}: ${tsType};\n`;
  });
  state.tsCode += "\t\t}\n";
  state.tsCode += "\t}\n";
  state.tsCode += "}\n";

  return nodes;
}

function doFileSystems(state, dts) {
	const root = dts.nodes['/'];
	const fstab = root.children.fstab;
  const nodes = []
  for (const name in fstab?.children)
    nodes.push(fstab.children[name]);

	state.hCode += `
#define kModZephyrFSCount (${nodes.length})
`;

	if (0 === nodes.length)
		return;

	state.hCode += `
#include <zephyr/fs/fs.h>

struct modZephyrFS {
	const char *label;
	struct fs_mount_t *mountpoint;
	uint8_t fsIndex;
};

extern const struct modZephyrFS *modZephyrGetFS(const char *label);

`;

  state.hCode += `
#ifndef MODDEF_ZEPHYR_FILESYSTEM_DEFAULT
	#define MODDEF_ZEPHYR_FILESYSTEM_DEFAULT "${nodes[0].label}"
#endif
`;

  nodes.forEach(node => {
    state.cCode += `
FS_FSTAB_DECLARE_ENTRY(DT_NODELABEL(${node.label}));
`;
  });

	state.cCode +=`
static const struct modZephyrFS gFS[] = {
`;

	nodes.forEach((node, index) => {
		state.cCode += `	{
		.label = "${node.label}",
		.mountpoint = &FS_FSTAB_ENTRY(DT_NODELABEL(${node.label})),
		.fsIndex = ${index}
	},
`;
	});

	state.cCode += `};

const struct modZephyrFS *modZephyrGetFS(const char *label)
{
	for (int i = 0; i < ARRAY_SIZE(gFS); i++) {
		if (0 == c_strcmp(gFS[i].label, label))
			return &gFS[i];
	}

	return C_NULL;
}
`;

  state.jsCode += `
import Modules from "modules";
Object.defineProperty(device, "files", {
	get() {
		return Modules.importNow("embedded:storage/files");
	}
});
`;

  state.tsCode += `
declare module "embedded:provider/builtin" {
	import {Directory} from "embedded:storage/files"

  interface Device {
		files: Directory
	}
}
`;
}

function doFlash(state, dts) {
  state.jsCode += `
import flash from "embedded:storage/flash";
device.flash = flash;
`;

  state.tsCode += `
declare module "embedded:provider/builtin" {
	import flash from "embedded:storage/flash";

	interface Device {
		flash: typeof flash
	}
}
`;
}

function doKeyValue(state, dts) {
  state.jsCode += `
import keyValue from "embedded:storage/key-value";
device.keyValue = keyValue;
`;

  state.tsCode += `
declare module "embedded:provider/builtin" {
	import keyValue from "embedded:storage/key-value";

	interface Device {
		keyValue: typeof keyValue
	}
}
`;
}

/*
  probably needs changes for non-Espressif silicon
*/
function doNetworkInterfaces(state, dts) {
	const root = dts.nodes['/'];
  const nics = [];

  if (root.children.wifi && ("y" === state.zephyrConfig.get("CONFIG_WIFI"))) {
    const status = root.children.wifi.properties.status?.value?.value ?? "okay";
    if ("okay" === status) {
      nics.push({
        label: root.children.wifi.label,
        kind: "wifi",
        name: `WiFi`,
        import: "embedded:network/interface/wifi",
      });
    }
  }

  if (root.children.eth && ("y" === state.zephyrConfig.get("CONFIG_NET_L2_ETHERNET"))) {
    const status = root.children.wifi.properties.status?.value?.value ?? "okay";
    if ("okay" === status) {
      nics.push({
        label: root.children.eth.label,
        kind: "ethernet",
        name: `Ethernet`,
        import: "embedded:network/interface/ethernet",
      });
    }
  }

  if (0 === nics.length)
    return;

  state.jsCode +=
`device.network ??= {};
device.network.interface ??= {};
`;

  nics.forEach(nic => {
    state.jsCode += `
import ${nic.name} from "${nic.import}";
device.network.interface.${nic.label} = Object.freeze({io: ${nic.name}, kind: "${nic.kind}"});

`;

    state.tsCode += `
declare module "embedded:provider/builtin" {
  import ${nic.name} from "${nic.import}";
	import type {PortSpecifier} from "embedded:io/_common";

  interface DeviceNetwork {
    interface: {
      ${nic.label}: {io: typeof ${nic.name}, kind: string, port: PortSpecifier};
    }
  }
}
`;
  });
}


/**
 * Device Tree Source (.dts) Parser
 * Optimized with lookup tables for character classification
 * Mostly generated by claude.ai - September 2025
 */

// ASCII code constants
const SPACE = 32;
const TAB = 9;
const LF = 10;
const CR = 13;
const QUOTE = 34;
const HASH = 35;
const AMPERSAND = 38;
const COMMA = 44;
const MINUS = 45;
const DOT = 46;
const SLASH = 47;
const ZERO = 48;
const NINE = 57;
const COLON = 58;
const SEMICOLON = 59;
const LT = 60;
const EQUALS = 61;
const GT = 62;
const AT = 64;
const A_UPPER = 65;
const F_UPPER = 70;
const Z_UPPER = 90;
const UNDERSCORE = 95;
const A_LOWER = 97;
const F_LOWER = 102;
const Z_LOWER = 122;
const LBRACE = 123;
const RBRACE = 125;

// Character type flags
const WHITESPACE = 1;
const DIGIT = 2;
const HEX_DIGIT = 4;
const IDENTIFIER_START = 8;
const IDENTIFIER_CHAR = 16;

// Create lookup table for character classification
const CHAR_TABLE = new Uint8Array(128);

// Initialize character lookup table
function initCharTable() {
  // Whitespace characters
  CHAR_TABLE[SPACE] |= WHITESPACE;
  CHAR_TABLE[TAB] |= WHITESPACE;
  CHAR_TABLE[LF] |= WHITESPACE;
  CHAR_TABLE[CR] |= WHITESPACE;
  
  // Digits (0-9)
  for (let i = ZERO; i <= NINE; i++) {
    CHAR_TABLE[i] |= DIGIT | HEX_DIGIT | IDENTIFIER_CHAR;
  }
  
  // Hex digits (A-F, a-f)
  for (let i = A_UPPER; i <= F_UPPER; i++) {
    CHAR_TABLE[i] |= HEX_DIGIT | IDENTIFIER_START | IDENTIFIER_CHAR;
  }
  for (let i = A_LOWER; i <= F_LOWER; i++) {
    CHAR_TABLE[i] |= HEX_DIGIT | IDENTIFIER_START | IDENTIFIER_CHAR;
  }
  
  // Remaining uppercase letters (G-Z)
  for (let i = F_UPPER + 1; i <= Z_UPPER; i++) {
    CHAR_TABLE[i] |= IDENTIFIER_START | IDENTIFIER_CHAR;
  }
  
  // Remaining lowercase letters (g-z)
  for (let i = F_LOWER + 1; i <= Z_LOWER; i++) {
    CHAR_TABLE[i] |= IDENTIFIER_START | IDENTIFIER_CHAR;
  }
  
  // Special identifier characters
  CHAR_TABLE[UNDERSCORE] |= IDENTIFIER_START | IDENTIFIER_CHAR;
  CHAR_TABLE[HASH] |= IDENTIFIER_START | IDENTIFIER_CHAR;
  CHAR_TABLE[COMMA] |= IDENTIFIER_CHAR;
  CHAR_TABLE[DOT] |= IDENTIFIER_CHAR;
  CHAR_TABLE[MINUS] |= IDENTIFIER_CHAR;
  CHAR_TABLE[AT] |= IDENTIFIER_CHAR;
}

initCharTable();

class DTSParser {
  constructor() {
    this.content = '';
    this.pos = 0;
    this.length = 0;
  }

  /**
   * Parse DTS content
   */
  parse(content) {
    this.content = content;
    this.contentS = new String(content);		// to avoid temporary instances
    this.pos = 0;
    this.length = content.length;
    
    const result = {
      version: null,
      nodes: {},
      includes: [],
      comments: []
    };

    // Extract version quickly
    const versionIndex = content.indexOf('/dts-v1/');
    if (versionIndex !== -1) {
      result.version = 'v1';
    }

    // Skip to root node
    this.skipToRoot();
    
    if (this.pos < this.length && this.content.charCodeAt(this.pos) === SLASH) {
      const nextCode = this.pos + 1 < this.length ? this.content.charCodeAt(this.pos + 1) : 0;
      if (nextCode !== 42) { // not '/*'
        const rootNode = this.parseNode();
        if (rootNode) {
          result.nodes[rootNode.name] = rootNode;
        }
      }
    }

    return result;
  }

  isWhitespace(code) {
    return CHAR_TABLE[code] & WHITESPACE;
  }
  isDigit(code) {
    return CHAR_TABLE[code] & DIGIT;
  }
  isHexDigit(code) {
    return CHAR_TABLE[code] & HEX_DIGIT;
  }
  isIdentifierStart(code) {
    return CHAR_TABLE[code] & IDENTIFIER_START;
  }
  isIdentifierChar(code) {
    return CHAR_TABLE[code] & IDENTIFIER_CHAR;
  }

  skipWhitespace() {
	const length = this.length, content = this.contentS;
	let pos = this.pos;
    while (pos < length) {
      const code = content.charCodeAt(pos);
      
      if (this.isWhitespace(code)) {
        pos++;
        continue;
      }
      
      if (code === SLASH && pos + 1 < length) {
        const nextCode = content.charCodeAt(pos + 1);
        
        if (nextCode === 42) { // '/*'
          pos += 2;
          while (pos < length - 1) {
            if (content.charCodeAt(pos) === 42 && 
                content.charCodeAt(pos + 1) === SLASH) {
              pos += 2;
              break;
            }
            pos++;
          }
          continue;
        }
        
        if (nextCode === SLASH) { // '//'
          pos += 2;
          while (pos < length && content.charCodeAt(pos) !== LF) {
            pos++;
          }
          continue;
        }
      }
      
      break;
    }
    this.pos = pos;
  }

  skipToRoot() {
    let searchPos = 0;
    while (searchPos < this.length) {
      const slashPos = this.content.indexOf('/', searchPos);
      if (slashPos === -1) break;
      
      // Check if this is a root node (/ followed by whitespace then {)
      let testPos = slashPos + 1;
      while (testPos < this.length && this.isWhitespace(this.content.charCodeAt(testPos))) {
        testPos++;
      }
      
      if (testPos < this.length && this.content.charCodeAt(testPos) === LBRACE) {
        this.pos = slashPos;
        return;
      }
      
      searchPos = slashPos + 1;
    }
  }

  parseNode() {
    this.skipWhitespace();
    
    let nodeName = '';
    let nodeLabel = null;
    const labels = [];
    
    // Parse node identifier
    if (this.content.charCodeAt(this.pos) === SLASH) {
      nodeName = '/';
      this.pos++;
    } else {
      const startPos = this.pos;
      while (this.pos < this.length && this.isIdentifierChar(this.content.charCodeAt(this.pos))) {
        this.pos++;
      }
      nodeName = this.content.slice(startPos, this.pos);
    }
    
    this.skipWhitespace();
    
    // Handle multiple labels (e.g., label1: label2: nodename)
    while (this.pos < this.length && this.content.charCodeAt(this.pos) === COLON) {
      labels.push(nodeName);
      this.pos++; // consume ':'
      this.skipWhitespace();
      
      const startPos = this.pos;
      while (this.pos < this.length && this.isIdentifierChar(this.content.charCodeAt(this.pos))) {
        this.pos++;
      }
      nodeName = this.content.slice(startPos, this.pos);
      this.skipWhitespace();
    }
    
    // Set the primary label (first one if multiple)
    nodeLabel = labels.length > 0 ? labels[0] : null;
    
    if (this.content.charCodeAt(this.pos) !== LBRACE) {
      return null;
    }
    this.pos++; // consume '{'
    
    const node = {
      name: nodeName,
      label: nodeLabel,
      properties: {},
      children: {}
    };
    
    // Add all labels if multiple exist
    if (labels.length > 1) {
      node.labels = labels;
    }
    
    // Parse node contents
    while (this.pos < this.length) {
      this.skipWhitespace();
      
      if (this.content.charCodeAt(this.pos) === RBRACE) {
        this.pos++;
        break;
      }
      
      // Parse identifier
      const startPos = this.pos;
      while (this.pos < this.length && this.isIdentifierChar(this.content.charCodeAt(this.pos))) {
        this.pos++;
      }
      
      if (this.pos === startPos) {
        this.pos++; // skip invalid character
        continue;
      }
      
      const identifier = this.content.slice(startPos, this.pos);
      this.skipWhitespace();
      
      // Quick check for child node vs property
      const nextCode = this.content.charCodeAt(this.pos);
      
      if (nextCode === COLON || nextCode === LBRACE) {
        // Child node - backtrack and parse
        this.pos = startPos;
        const childNode = this.parseNode();
        if (childNode) {
          node.children[childNode.name] = childNode;
        }
      } else {
        // Property
        const property = this.parseProperty(identifier);
        if (property) {
          node.properties[property.name] = property;
        }
      }
    }
    
    return node;
  }

  parseProperty(name) {
    this.skipWhitespace();
    
    if (this.pos >= this.length) {
      throw new Error(`Unexpected end of input while parsing property '${name}' at position ${this.pos}`);
    }
    
    const property = {
      name: name,
      value: null
    };
    
    if (this.content.charCodeAt(this.pos) === EQUALS) {
      this.pos++;
      this.skipWhitespace();
      
      if (this.pos >= this.length) {
        throw new Error(`Unexpected end of input after '=' for property '${name}' at position ${this.pos}`);
      }
      
      try {
        property.value = this.parseValue();
      } catch (error) {
        throw new Error(`Failed to parse value for property '${name}': ${error.message}`);
      }
    }
    
    // Skip to semicolon
    while (this.pos < this.length && this.content.charCodeAt(this.pos) !== SEMICOLON) {
      this.pos++;
    }
    
    if (this.pos >= this.length) {
      throw new Error(`Missing semicolon for property '${name}' - unexpected end of input at position ${this.pos}`);
    }
    
    this.pos++; // consume semicolon
    return property;
  }

  parseValue() {
    this.skipWhitespace();
    const values = [];
    
    while (this.pos < this.length) {
      this.skipWhitespace();
      
      const code = this.content.charCodeAt(this.pos);
      
      if (code === SEMICOLON || code === RBRACE) {
        break;
      }
      
      if (code === QUOTE) {
        values.push({ type: 'string', value: this.parseString() });
      } else if (code === LT) {
        values.push({ type: 'array', value: this.parseArray() });
      } else if (code === AMPERSAND) {
        this.pos++;
        this.skipWhitespace();
        values.push({ type: 'reference', value: this.parseIdentifier() });
      } else if (this.isDigit(code) || (code === ZERO && this.pos + 1 < this.length && this.content.charCodeAt(this.pos + 1) === 120)) { // 'x'
        values.push({ type: 'number', value: this.parseNumber() });
      } else if (this.isIdentifierStart(code)) {
        values.push({ type: 'identifier', value: this.parseIdentifier() });
      } else if (code === COMMA) {
        this.pos++;
        this.skipWhitespace();
      } else {
        this.pos++;
      }
    }
    
    if (values.length === 0) return null;
    if (values.length === 1) return values[0];
    
    const types = values.map(v => v.type);
    if (types.every(t => t === 'string')) {
      return { type: 'string-array', value: values.map(v => v.value) };
    } else {
      return { type: 'mixed-array', value: values };
    }
  }

  parseString() {
    this.pos++; // consume opening quote
    const start = this.pos, length = this.length, content = this.contentS;
    
    while (this.pos < length) {
	  const c = content.charCodeAt(this.pos);
	  if (c === QUOTE)
	    break;
      if (c === 92) { // backslash
        this.pos += 2; // skip escaped character
      } else {
        this.pos++;
      }
    }
    
    return content.slice(start, this.pos++); // consume closing quote
  }

  parseArray() {
    this.pos++; // consume '<'
    this.skipWhitespace();
    const values = [];
    
    while (this.pos < this.length) {
      this.skipWhitespace();
      
      if (this.content.charCodeAt(this.pos) === GT) {
        this.pos++;
        break;
      }
      
      const code = this.content.charCodeAt(this.pos);
      
      if (code === AMPERSAND) {
        this.pos++;
        this.skipWhitespace();
        values.push('&' + this.parseIdentifier());
      } else if (this.isDigit(code) || (code === ZERO && this.pos + 1 < this.length && this.content.charCodeAt(this.pos + 1) === 120)) { // 'x'
        values.push(this.parseNumber());
      } else if (this.isIdentifierStart(code)) {
        values.push(this.parseIdentifier());
      } else {
        this.pos++;
      }
      
      this.skipWhitespace();
    }
    
    return values;
  }

  parseNumber() {
    const startPos = this.pos;
    
    if (this.content.charCodeAt(this.pos) === ZERO && 
        this.pos + 1 < this.length && 
        this.content.charCodeAt(this.pos + 1) === 120) { // 'x'
      // Hex number
      this.pos += 2;
      while (this.pos < this.length && this.isHexDigit(this.content.charCodeAt(this.pos))) {
        this.pos++;
      }
    } else {
      // Decimal number
      while (this.pos < this.length && this.isDigit(this.content.charCodeAt(this.pos))) {
        this.pos++;
      }
    }
    
    return this.content.slice(startPos, this.pos);
  }

  parseIdentifier() {
    const startPos = this.pos;
    
    while (this.pos < this.length && this.isIdentifierChar(this.content.charCodeAt(this.pos))) {
      this.pos++;
    }
    
    return this.content.slice(startPos, this.pos);
  }

  static toDTS(tree) {
    let output = '';
    
    if (tree.version) {
      output += `/dts-${tree.version}/;\n\n`;
    }
    
    for (const [, node] of Object.entries(tree.nodes)) {
      output += DTSParser.nodeToString(node, 0);
    }
    
    return output;
  }

  static nodeToString(node, indent = 0) {
    const indentStr = '\t'.repeat(indent);
    
    let nodeDeclaration;
    if (node.label) {
      nodeDeclaration = `${indentStr}${node.label}: ${node.name} {\n`;
    } else {
      nodeDeclaration = `${indentStr}${node.name} {\n`;
    }
    
    let output = nodeDeclaration;
    
    // Properties
    for (const [propName, prop] of Object.entries(node.properties)) {
      output += `${indentStr}\t${propName}`;
      if (prop.value) {
        output += ` = ${DTSParser.valueToString(prop.value)}`;
      }
      output += ';\n';
    }
    
    // Child nodes
    const childEntries = Object.entries(node.children);
    for (let i = 0; i < childEntries.length; i++) {
      const [, child] = childEntries[i];
      if (i === 0 && Object.keys(node.properties).length > 0) {
        output += '\n';
      }
      output += DTSParser.nodeToString(child, indent + 1);
      if (i < childEntries.length - 1) {
        output += '\n';
      }
    }
    
    output += `${indentStr}};\n`;
    return output;
  }

  static valueToString(value) {
    if (!value) return '';
    
    switch (value.type) {
      case 'string':
        return `"${value.value}"`;
      case 'string-array':
        return value.value.map(v => `"${v}"`).join(',\n\t\t             ');
      case 'number':
        return value.value;
      case 'reference':
        return `&${value.value}`;
      case 'array':
        return `< ${value.value.join(' ')} >`;
      case 'identifier':
        return value.value;
      case 'mixed-array':
        return value.value.map(v => DTSParser.valueToString(v)).join(',\n\t\t             ');
      default:
        return String(value.value || '');
    }
  }

  static filter(tree, callback) {
    const selected = [];
    
    function traverse(node) {
      if (callback(node))
        selected.push(node);
      
      for (const child of Object.values(node.children))
        traverse(child);
    }
    
    for (const node of Object.values(tree.nodes))
      traverse(node);
    
    return selected;
  }
}
