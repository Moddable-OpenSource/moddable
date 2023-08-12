/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
	This application demonstrates how to use the Monitor object to trigger wakeup from deep sleep on a button press.
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the application re-launches and blinks the LED if the button was pressed to wakeup the device.
	Press the button connected to the digital input pin to wakeup the device.
*/

import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";
import {Sleep} from "sleep";
import Timer from "timer";
import config from "mc/config";

const wakeup_pin = 17;
const led = new Host.LED.Default;
 
led.write(1);

let monitor = new Monitor({
	pin: wakeup_pin,
	mode: Digital.InputPullUp,
	wakeEdge: Digital.WakeOnFall,
	edge: Monitor.Rising | Monitor.Falling,
	onWake() {
		for (let i = 0; i < 10; ++i) {
			led.write(0);
			Timer.delay(50);
			led.write(1);
			Timer.delay(50);
		}
	}
});

Timer.set(() => {
	led.write(0);
	led.close();
	Sleep.deep();
}, 3000);

