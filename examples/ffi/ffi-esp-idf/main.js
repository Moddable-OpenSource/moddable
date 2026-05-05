/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import FFI from "mc/ffi";
import {gpio_io_config_t} from "gpio_t"
import Timer from "timer"

const Natives = new FFI;

const targetGPIO = 0;

// get / set drive capability
let uint32 = new Uint32Array(1);
Natives.gpio_get_drive_capability(targetGPIO, uint32.buffer);
trace(`GPIO drive ${uint32[0]}\n`);		// default - expect 2 (GPIO_DRIVE_CAP_DEFAULT)

Natives.gpio_set_drive_capability(targetGPIO, 3 /* GPIO_DRIVE_CAP_3 - strongest */);
Natives.gpio_get_drive_capability(targetGPIO, uint32.buffer);
trace(`GPIO drive ${uint32[0]}\n`);

// disable internal pull-up
Natives.gpio_pullup_dis(targetGPIO);

// dump configuration
let config = new gpio_io_config_t;
Natives.gpio_get_io_config(targetGPIO, config.buffer);
trace(JSON.stringify(config, undefined, "  "), "\n");

// reset and dump configuration to see what changed
Natives.gpio_reset_pin(targetGPIO);

Natives.gpio_get_io_config(targetGPIO, config.buffer);
trace(JSON.stringify(config, undefined, "  "), "\n");

// configure as input with pull-up
Natives.gpio_set_direction(targetGPIO, 1 /* GPIO_MODE_INPUT */);
Natives.gpio_pullup_en(targetGPIO);

// poll to log button state changes
trace(`Monitoring GPIO ${targetGPIO} for changes\n`);
let last = undefined;
Timer.repeat(() => {
	const level = Natives.gpio_get_level(targetGPIO);
	if (level === last)
		return;

	last = level;
	trace(level, "\n");
}, 10)
