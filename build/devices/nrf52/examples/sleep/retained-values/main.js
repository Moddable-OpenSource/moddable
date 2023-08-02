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
	This application demonstrates how to retain 32-bit values in RAM across System Off power saving mode (deep sleep).
	The device is woken up from a digital input.
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the LED blinks if the retained values are valid.
	Press the button connected to the digital input pin to wakeup the device.
*/

import Sleep from "sleep";
import Digital from "pins/digital";
import Timer from "timer";
import config from "mc/config";

const wakeup_pin = config.button1_pin;
const led = new Host.LED.Default;

// Turn on LED upon wakeup
led.write(1);

let digital = new Digital({
	pin: wakeup_pin,
	mode: Digital.InputPullUp,
	wakeEdge: Digital.WakeOnFall,
	onWake() {
	}
});

// Check if retained values are valid
let valid = true;
for (let i = 0; i < 32; ++i) {
	let value = Sleep.getRetainedValue(i);
	if (i + 1 != value) {
		valid = false;
		break;
	}
}

// Blink LED to confirm retained values
if (valid) {
	for (let i = 0; i < 10; ++i) {
		led.write(0);
		Timer.delay(50);
		led.write(1);
		Timer.delay(50);
	}
}

// Retain values and sleep
else {
	Timer.set(() => {
		led.write(0);
		led.close();
		for (let i = 0; i < 32; ++i)
			Sleep.setRetainedValue(i, i + 1);
		Sleep.deep();
	}, 3000);
}
