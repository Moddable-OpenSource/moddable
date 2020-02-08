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
	This application demonstrates how to use the WakeableAnalog object to determine if the device woke up from deep sleep due to hard reset or another trigger.
	The nRF52 only exits deep sleep from a pre-configured digital, analog, or NFC trigger. Also from hard reset.
	Upon wakeup the LED blinks if wakeup was due to a pre-configured trigger.
	The device is woken up from an analog input or hard reset.
	The application turns on the LED while running and turns off the LED when asleep.
	Adjust the value of the analog input pin or press the reset button to wakeup the device.
*/

import WakeableAnalog from "builtin/wakeableanalog";
import Digital from "pins/digital";
import Sleep from "sleep";
import Timer from "timer";
import config from "mc/config";

const wakeup_pin = 1;	// channel 1, P.03
const led_pin = config.led1_pin;
const ON = 1;
const OFF = 0;

// Turn on LED upon wakeup
Digital.write(led_pin, ON);

//let wakeable = new WakeableAnalog({ pin: "RST" });
//if (wakeable.read())
//	blink();

let wakeable = new WakeableAnalog({ pin:wakeup_pin, mode:"crossing", value:512 });
if ("analog" == wakeable.wakeupReason)
	blink();

let count = 3;
Timer.repeat(id => {
	if (0 == count) {
		Timer.clear(id);
		
		// turn off led while asleep
		Digital.write(led_pin, OFF);
		
		Sleep.deep();
	}
	--count;
}, 1000);

function blink()
{
	for (let i = 0; i < 5; ++i) {
		Digital.write(led_pin, ON);
		Timer.delay(100);
		Digital.write(led_pin, OFF);
		Timer.delay(100);
	}
}
