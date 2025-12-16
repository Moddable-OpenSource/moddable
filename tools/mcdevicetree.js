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
				argi++
				if (argi >= argc)
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
	}

	run() {
		let dts = String.fromArrayBuffer(this.readFileBuffer(this.sourcePath));
		dts = dts.replace(/\/\*[\s\S]*?\*\//g, '') // Remove block comments
				.replace(/[ \t]+/g, ' ')           // Normalize whitespace
				.replace(/\n\s*\n\s*/g, '\n');     // Remove extra blank lines

		const parser = new DTSParser();
		const parsed = parser.parse(dts);

		const state = {
			cCode: "",
			hCode: "",
			jsCode: "",
			aliasTable: new Map,
			zephyrConfig: this.zephyrConfig
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

		doAliases(state, parsed);
		doGPIOBanks(state, parsed); 
		doGPIOs(state, parsed);

		doBus(state, parsed, {
			prefix: "i2c@",
			name: "I2C",
			header: "#include <zephyr/drivers/i2c.h>",
			static:
`import I2C from "embedded:io/i2c";
device.io.I2C = I2C;
// import SMBus from "embedded:io/smbus";
// device.io.SMBus = SMBus;

device.I2C = {};
`
		});

		doBus(state, parsed, {
			prefix: "serial@",
			name: "Serial",
			header: "#include <zephyr/drivers/uart.h>",
			static:
`import Serial from "embedded:io/serial";
device.io.Serial = Serial;

device.Serial = {};
`
		});

    const user = parsed.nodes['/'].children["zephyr,user"];
    const hasIOChannels = undefined !== user?.properties["io-channels"];

		if (("y" === state.zephyrConfig.get("CONFIG_ADC")) && hasIOChannels) {
			doBus(state, parsed, {
				prefix: "adc@",
				name: "Analog",
				header: "#include <zephyr/drivers/adc.h>",
				static:
`import Analog from "embedded:io/analog";
device.io.Analog = Analog;

device.Analog = {};
`
			});
		}

		doBus(state, parsed, {
			prefix: "pwm@",
			name: "PWM",
			header: "#include <zephyr/drivers/pwm.h>",
			static:
`import PWM from "embedded:io/pwm";
device.io.PWM = PWM;

device.PWM = {};
`
		});

    if ("y" === state.zephyrConfig.get("CONFIG_RTC")) {
      let rtcs = doBus(state, parsed, {
          prefix: "rtc@",
          name: "RTC",
          header: "#include <zephyr/drivers/rtc.h>",
          js: false,
        });

  
      if (rtcs?.length) {
        state.jsCode += `
import RTC from "embedded:RTC/zephyr-builtin";
device.rtc = {io: RTC, port: "${rtcs[0].label}"};
`;
        }
    }

    if ("y" === state.zephyrConfig.get("CONFIG_DISPLAY")) {
      doBus(state, parsed, {
        prefix: "display-controller@",
        name: "Display",
        hostProviderName: "display",
        header: "#include <zephyr/drivers/display.h>",
        static:
`import Display from "embedded:display/zephyr";
device.display = {};
`
      });
    }

    doNetworkInterfaces(state, parsed);

/*
doBus(state, parsed, {
			prefix: "spi@",
			name: "SPI",
			header: "#include <zephyr/drivers/spi.h>",
			static:
`import SPI from "embedded:io/spi";
device.io.SPI = SPI;

device.SPI = {};
`
		});
*/


    if ("y" === state.zephyrConfig.get("CONFIG_FILE_SYSTEM"))
      doFileSystems(state, parsed);

		state.hCode +=
`
#endif /* __MC_ZEPHYR_H__ */
`;

		state.jsCode += `
export default device;
`;

		const parts = {
				name: "mc.devicetree",
				directory: this.outputDirectory,
		};

		["h", "c", "js"].forEach(extension => {
			parts.extension = "." + extension;
			let output = new FILE(this.joinPath(parts), "wb");
			output.writeString(state[extension + "Code"]);
			output.close();
		});
	}
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
	let gpios = [];
	const root = dts.nodes['/'];

	const soc = root.children.soc;
	for (let what in soc.children) {
		if (what.startsWith("gpio@")) {
			const node = soc.children[what];
			const status = node.properties.status?.value?.value ?? "okay"
			if ("okay" !== status)
				continue;

			if ("raspberrypi,pico-gpio" === node.properties.compatible.value.value) {
				for (let what in node.children) {
					const gpio = node.children[what];
					if ("okay" !== (gpio.properties.status?.value?.value ?? "okay"))
						continue;
					gpios.push(gpio);
				}
			}
			else
				gpios.push(node);
		}
	}

	for (let what in soc.children) {
		if (!what.startsWith("pin-controller@"))
			continue;

		const pinController = soc.children[what] 
		for (const bus in pinController.children) {
			if (bus.startsWith("gpio@")) {
				const	gpio = pinController.children[bus];
				const status = gpio.properties.status?.value?.value ?? "okay"
				if ("okay" !== status)
					continue;
				gpios.push(gpio);
			}
		}
	}

	if ((0 === gpios.length) && soc.children.gpio) {
		for (let what in soc.children.gpio.children) {
			if (!what.startsWith("gpio@"))
				continue;

			const gpio = soc.children.gpio.children[what];
			const status = gpio.properties.status?.value?.value ?? "okay"
			if ("okay" !== status)
				continue;
			gpios.push(gpio);
		}
  }

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

	state.jsCode +=`
import Digital from "embedded:io/digital";
device.io.Digital = Digital;

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

/*
	state.cCode +=`
static const struct modZephyrGPIO gGPIO[] = {
`;

	gpios.forEach(gpio => {
		state.cCode += `	{${gpio.userName ? " // " + gpio.userName : ""}
		.name = "${gpio.name}",
		.dt = GPIO_DT_SPEC_GET(DT_PATH(${gpio.kind}, ${gpio.name}), gpios),
		.bankIndex = ${gpio.bankIndex}
	},
`;
	});

	state.cCode += `};

const struct modZephyrGPIO *modZephyrGetGPIO(const char *name)
{
	for (int i = 0; i < ARRAY_SIZE(gGPIO); i++) {
		if (0 == c_strcmp(gGPIO[i].name, name))
			return &gGPIO[i];
	}

	return C_NULL;
}
`;

	state.hCode += `

struct modZephyrGPIO {
	const char *name;
	const struct gpio_dt_spec dt;
	uint8_t bankIndex;
};

extern const struct modZephyrGPIO *modZephyrGetGPIO(const char *name);

`;
*/

	const kinds = new Set();
	gpios.forEach(gpio => {
		let mode = [], kindName, edge;
		if ("gpio-leds" == gpio.kind) {
			mode.push("Digital.Output");
			kindName = "led";
			//@@ flags OutputOpenDrain
			if (gpio.flags & (1 << 0))		// GPIO_ACTIVE_LOW
				mode.push("Digital.ActiveLow");
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

				if (("sw0" === alias) && ("gpio-keys" === gpio.kind))
					state.jsCode += `device.pin.button = {port: "${gpio.bus}", pin: ${gpio.pin}}\n`;
				if (("led0" === alias) && ("gpio-leds" === gpio.kind))
					state.jsCode += `device.pin.led = {port: "${gpio.bus}", pin: ${gpio.pin}}\n`;
			});
		}
	});
}

function doBus(state, dts, options) {
	let nodes = [];
	const root = dts.nodes['/'];
	const soc = root.children.soc;
	for (let what in soc.children) {
		if (what.startsWith(options.prefix)) {
			const node = soc.children[what];
			const status = node.properties.status?.value?.value ?? "okay"
			if ("okay" !== status)
				continue;
			nodes.push(node);
		}
	}

	state.hCode += `
#define kModZephyr${options.name}BusCount (${nodes.length})
`;

	if (0 === nodes.length)
		return;

	let busSpecific = "";
	if ("spi@" === options.prefix)
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
		if ("spi@" === options.prefix) {
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
		if ("serial@" === options.prefix) {
			const baud = node.properties["current-speed"]?.value?.value?.[0];
			if (baud)
				additional += `baud: ${parseInt(baud)}`;
		}
		state.jsCode += `device.${hostProviderName}.${node.label} = {io: ${options.name}, port: "${node.label}"${additional ? ", " + additional : ""}};\n`;
		for (let i = 1; i < node.labels?.length; i++) 
			state.jsCode += `device.${hostProviderName}.${node.labels[i]} = device.${hostProviderName}.${node.label};\n`;
	});

	// use first one as default - just a guess. always correct when there is just one bus. is there a "chosen" for this??
	state.jsCode += `device.${hostProviderName}.default = device.${hostProviderName}.${nodes[0].label};\n`;

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
device.network.interface.${nic.label} = {io: ${nic.name}, kind: "${nic.kind}"};

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
}
