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
	Upon wakeup, the application re-launches and blinks the LED if the analog value change woke up the device.
	Change the voltage connected to the analog input pin to wakeup the device.
*/

import {Sleep, AnalogDetectMode, ResetReason} from "sleep";
import Timer from "timer";

const wakeup_channel = 5;		// AIN5
const led = new Host.LED;

// Turn on LED upon wakeup
led.write(1);

// Blink LED upon wakeup from analog value trigger
if (ResetReason.LPCOMP == Sleep.resetReason) {
	for (let i = 0; i < 10; ++i) {
		led.write(0);
		Timer.delay(50);
		led.write(1);
		Timer.delay(50);
	}
}

Timer.set(() => {
	Sleep.wakeOnAnalog(wakeup_channel, { value:512, mode:AnalogDetectMode.Crossing });
	//Sleep.wakeOnAnalog(CHANNEL, { value:512, mode:AnalogDetectMode.Up });
	//Sleep.wakeOnAnalog(CHANNEL, { value:512, mode:mode:AnalogDetectMode.Down });
	led.write(0);
	Sleep.deep();
}, 3000);

