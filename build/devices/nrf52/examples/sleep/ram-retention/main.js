/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
/*
	This application demonstrates how to retain a buffer in RAM across System Off power saving mode (deep sleep).
	The device is woken up from a digital input.
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the LED blinks if the retention buffer contents are valid.
	Press the button connected to the digital input pin to wakeup the device.
*/

import Sleep from "sleep";
import Timer from "timer";
import config from "mc/config";

const wakeup_pin = config.button1_pin;
const led = new Host.LED;

// Turn on LED upon wakeup
led.write(1);

// Check if retained ram buffer is available
let buffer = Sleep.getRetainedBuffer();
let valid = true;

if (undefined !== buffer) {
	let retained = new Uint8Array(buffer);
	for (let i = 0; i < 100; ++i) {
		if (retained[i] != i) {
			// Turn off LED if retention buffer invalid
			led.write(0);
			valid = false;
			break;
		}
	}
}
else
	valid = false;

// Blink LED to confirm retention buffer
if (valid) {
	for (let i = 0; i < 10; ++i) {
		led.write(0);
		Timer.delay(50);
		led.write(1);
		Timer.delay(50);
	}
	Sleep.clearRetainedBuffer();
}
else {
	Timer.set(() => {
		Sleep.install(preSleep);
		Sleep.deep();
	}, 3000);
}

function preSleep() {
	led.write(0);
	Sleep.wakeOnDigital(wakeup_pin);
	let retained = new Uint8Array(100);
	for (let i = 0; i < 100; ++i)
		retained[i] = i;
	Sleep.setRetainedBuffer(retained.buffer);
}
