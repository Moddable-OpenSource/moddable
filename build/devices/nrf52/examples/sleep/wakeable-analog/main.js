/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
	This application demonstrates how to use the WakeableAnalog object to determine if the device woke up from deep sleep due to hard reset or another trigger.
	The nRF52 only exits deep sleep from a pre-configured digital, analog, or NFC trigger. Also from hard reset.
	Upon wakeup the LED blinks if wakeup was due to a pre-configured trigger.
	The device is woken up from an analog input or hard reset.
	The application turns on the LED while running and turns off the LED when asleep.
	Adjust the value of the analog input pin or press the reset button to wakeup the device.
*/

import WakeableAnalog from "builtin/wakeableanalog";
import Sleep from "sleep";
import Timer from "timer";

const wakeup_pin = 1;	// channel 1, P.03
const led = new Host.LED.Default;

// Turn on LED upon wakeup
led.write(1);

//let wakeable = new WakeableAnalog({ pin: "RST" });
//if (wakeable.read())
//	blink();

let wakeable = new WakeableAnalog({ pin:wakeup_pin, mode:"crossing", value:512 });
if ("analog" == wakeable.wakeupReason) {
	for (let i = 0; i < 10; ++i) {
		led.write(1);
		Timer.delay(50);
		led.write(0);
		Timer.delay(50);
	}
}

Timer.set(() => {
	led.write(0);
	led.close();
	Sleep.deep();
}, 3000);

