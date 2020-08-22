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
	This application demonstrates how to use the Sleep object to put the device into System Off power saving mode (deep sleep).
	The device is woken up from a digital input (button).
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the application re-launches and blinks the LED if the button was pressed to wakeup the device.
	Press the button connected to the digital input pin to wakeup the device.
*/

import {Sleep, ResetReason} from "sleep";
import Timer from "timer";
import Digital from "pins/digital";
import config from "mc/config";

const wakeup_pin = config.button1_pin;
const led_pin = config.led1_pin;
const ON = 1;
const OFF = 0;

// Turn on LED upon wakeup
Digital.write(led_pin, ON);

// Blink LED upon wakeup from digital input pin
if (ResetReason.GPIO == Sleep.resetReason) {
	for (let i = 0; i < 10; ++i) {
		Digital.write(led_pin, OFF);
		Timer.delay(50);
		Digital.write(led_pin, ON);
		Timer.delay(50);
	}
}

let count = 3;
Timer.repeat(id => {
	if (0 == count) {
		Timer.clear(id);
		
		// wakeup on pin
		Sleep.wakeOnDigital(wakeup_pin);

		// turn off led while asleep
		Digital.write(led_pin, OFF);
		
		Sleep.deep();
	}
	if (count > 1)
		trace(`Going to deep sleep in ${count - 1} seconds...\n`);
	else
		trace(`Good night. Press the button connected to pin ${wakeup_pin} to wake me up.\n\n`);
	--count;
}, 1000);

function valueToString(obj, value) {
	let result = Object.keys(obj).find(element => obj[element] == value);
	return (result ? result : "Unknown");
}
