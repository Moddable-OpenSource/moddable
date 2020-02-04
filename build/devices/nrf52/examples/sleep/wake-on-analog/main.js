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
	This application demonstrates how to use the Sleep object to trigger wakeup on analog change detection.
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the application re-launches and the reset reason is traced to the console.
	Change the voltage connected to the analog input pin to wakeup the device.
*/

import {Sleep, AnalogDetectMode, ResetReason} from "sleep";
import Timer from "timer";
import Analog from "pins/analog";
import Digital from "pins/digital";
import config from "mc/config";

const wakeup_channel = 5;		// AIN5
const led_pin = config.led1_pin;
const ON = 1;
const OFF = 0;

let str = valueToString(ResetReason, Sleep.resetReason);
let value = Analog.read(wakeup_channel);

trace(`Good morning. Reset reason: ${str}\n`);
trace(`Analog value: ${value}\n`);

// Turn on LED upon wakeup
Digital.write(led_pin, ON);

let count = 3;
Timer.repeat(id => {
	if (0 == count) {
		Timer.clear(id);
		
		// wakeup on analog value change
		Sleep.wakeOnAnalog(wakeup_channel, { value:512, mode:AnalogDetectMode.Crossing });
		//Sleep.wakeOnAnalog(CHANNEL, { value:512, mode:AnalogDetectMode.Up });
		//Sleep.wakeOnAnalog(CHANNEL, { value:512, mode:mode:AnalogDetectMode.Down });

		// turn off led while asleep
		Digital.write(led_pin, OFF);
		
		Sleep.deep();
	}
	if (count > 1)
		trace(`Going to deep sleep in ${count - 1} seconds...\n`);
	else
		trace(`Good night. Adjust analog channel ${wakeup_channel} to wake me up.\n\n`);
	--count;
}, 1000);

function valueToString(obj, value) {
	let result = Object.keys(obj).find(element => obj[element] == value);
	return (result ? result : "Unknown");
}
