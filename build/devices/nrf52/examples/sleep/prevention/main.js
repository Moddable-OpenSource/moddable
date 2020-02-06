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
	This application demonstrates how to use the Sleep object to prevent the device from entering System Off power saving mode (deep sleep).
	The LED blinks when the app prevents the device from entering deep sleep;
	Upon wakeup, the application re-launches and the reset reason is traced to the console.
	Press the device reset button to wakeup from deep sleep.
*/

import {Sleep, ResetReason} from "sleep";
import Timer from "timer";
import Digital from "pins/digital";
import config from "mc/config";

const led_pin = config.led1_pin;
const ON = 1;
const OFF = 0;

let str = valueToString(ResetReason, Sleep.resetReason);
trace(`Good morning. Reset reason: ${str}\n`);

// Turn on LED upon wakeup
Digital.write(led_pin, ON);

// Prevent deep sleep
Sleep.prevent();

// Queue sleep request for when we allow sleep
Sleep.deep();
 
let count = 3;
Timer.repeat(id => {
	if (0 == count) {
		Timer.clear(id);

		// turn off led while asleep
		Digital.write(led_pin, OFF);

		Sleep.allow();
	}
	if (count > 1)
		trace(`Going to deep sleep in ${count - 1} seconds...\n`);
	else
		trace(`Good night. Press the reset button to wake me up.\n\n`);
	--count;
}, 1000);

function valueToString(obj, value) {
	let result = Object.keys(obj).find(element => obj[element] == value);
	return (result ? result : "Unknown");
}
