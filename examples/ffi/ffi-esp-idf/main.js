import FFI from "mc/ffi";
import {gpio_io_config_t} from "gpio_t"
import Timer from "timer"

const gpio = new FFI;

const targetGPIO = 0;

// get / set drive capability

let uint32 = new Uint32Array(1);
gpio.gpio_get_drive_capability(targetGPIO, uint32.buffer);
trace(`GPIO drive ${uint32[0]}\n`);		// default - expect 2 (GPIO_DRIVE_CAP_DEFAULT)

gpio.gpio_set_drive_capability(targetGPIO, 3 /* GPIO_DRIVE_CAP_3 - strongest */);
gpio.gpio_get_drive_capability(targetGPIO, uint32.buffer);
trace(`GPIO drive ${uint32[0]}\n`);

// disable internal pull-up
gpio.gpio_pullup_dis(targetGPIO);

// dump configuration
let config = new gpio_io_config_t;
gpio.gpio_get_io_config(targetGPIO, config.buffer);
trace(JSON.stringify(config, undefined, "  "), "\n");

// reset and dump configuration to see what changed
gpio.gpio_reset_pin(targetGPIO);

gpio.gpio_get_io_config(targetGPIO, config.buffer);
trace(JSON.stringify(config, undefined, "  "), "\n");

// configure as input with pull-up
gpio.gpio_set_direction(targetGPIO, 1 /* GPIO_MODE_INPUT */);
gpio.gpio_pullup_en(targetGPIO);

// poll to log button state changes
trace(`Monitoring GPIO ${targetGPIO} for changes\n`);
let last = undefined;
Timer.repeat(() => {
	const level = gpio.gpio_get_level(targetGPIO);
	if (level === last)
		return;

	last = level;
	trace(level, "\n");
}, 10)
