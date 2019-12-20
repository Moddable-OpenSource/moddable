/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
	The application turns on the LED1 while running and turns off the LED1 when asleep.
	Upon wakeup, all LEDs are turned on if the retention buffer contents are valid.
	If the retention buffer contents are invalid, LED4 is turned on.
	Press the button connected to the digital input PIN to wakeup the device.
*/

import {Sleep, ResetReason} from "sleep";
import Timer from "timer";
import Digital from "pins/digital";

const PIN = 25;		// Button 4 on nRF52840-DK
const LED1 = 13;	// LED1 on nRF52840-DK
const LED2 = 14;	// LED2 on nRF52840-DK
const LED3 = 15;	// LED3 on nRF52840-DK
const LED4 = 16;	// LED4 on nRF52840-DK

const ON = 0;		// active low
const OFF = 1;

let str = valueToString(ResetReason, Sleep.resetReason);

trace(`Good morning. Reset reason: ${str}\n`);

allLEDs(OFF);

// Turn on LED1 upon wakeup
Digital.write(LED1, ON);

// Check if retained ram buffer is available
let buffer = Sleep.getRetainedBuffer();

if (undefined !== buffer) {
	let retained = new Uint8Array(buffer);
	let valid = true;
	for (let i = 0; i < 100; ++i) {
		if (retained[i] != i) {
			Digital.write(LED4, ON);
			valid = false;
			break;
		}
	}
			
	// Turn on all LEDs to confirm retention buffer
	if (valid) {
		allLEDs(ON);
		Sleep.clearRetainedBuffer();
		trace(`Retention buffer read and okay.\n`);
	}
}

// No retained ram buffer is available. Retain a buffer and sleep.
else {
	let count = 3;
	Timer.repeat(id => {
		if (0 == count) {
			Timer.clear(id);
			Sleep.install(preSleep);
			Sleep.deep();
		}
		if (count > 1)
			trace(`Going to deep sleep in ${count - 1} seconds...\n`);
		else
			trace(`Good night. Press the button connected to pin ${PIN} to wake me up.\n\n`);
		--count;
	}, 1000);
}

function preSleep() {
	// Turn off LEDS while asleep
	allLEDs(OFF);

	// Wakeup on digital pin
	Sleep.wakeOnDigital(PIN);
	
	// Retain buffer
	let retained = new Uint8Array(100);
	for (let i = 0; i < 100; ++i)
		retained[i] = i;
	Sleep.setRetainedBuffer(retained.buffer);
}

function allLEDs(mode) {
	Digital.write(LED1, mode);
	Digital.write(LED2, mode);
	Digital.write(LED3, mode);
	Digital.write(LED4, mode);
}

function valueToString(obj, value) {
	let result = Object.keys(obj).find(element => obj[element] == value);
	return (result ? result : "Unknown");
}
