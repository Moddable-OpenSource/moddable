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
	Upon wakeup, the application re-launches and blinks the LED if the reset pin was pressed to wakeup the device.
	Press the device reset button to wakeup from deep sleep.
*/

import {Sleep, ResetReason} from "sleep";
import Timer from "timer";

const led = new Host.LED.Default;

// Turn on LED upon wakeup
led.write(1);

// Blink LED upon wakeup from reset pin
if (ResetReason.RESETPIN == Sleep.resetReason) {
	for (let i = 0; i < 10; ++i) {
		led.write(0);
		Timer.delay(50);
		led.write(1);
		Timer.delay(50);
	}
}

Timer.set(() => {
	led.write(0);
	led.close();
	Sleep.deep();
}, 3000);
