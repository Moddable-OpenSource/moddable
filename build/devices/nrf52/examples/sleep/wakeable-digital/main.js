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
	This application demonstrates how to use the WakeableDigital object to determine if the device woke up from deep sleep due to hard reset or another trigger.
	The nRF52 only exits deep sleep from a pre-configured digital, analog, or NFC trigger. Also from hard reset.
	Upon wakeup the LED blinks if wakeup was due to a pre-configured trigger.
	The device is woken up from a digital input or hard reset.
	The application turns on the LED while running and turns off the LED when asleep.
	Press the button connected to the digital input pin or the reset button to wakeup the device.
*/

import WakeableDigital from "builtin/wakeabledigital";
import Sleep from "sleep";
import Timer from "timer";
import config from "mc/config";

const wakeup_pin = config.button1_pin;
const led = new Host.LED.Default;

// Turn on LED upon wakeup
led.write(1);

//let wakeable = new WakeableDigital({ pin: "RST" });
//if (wakeable.read())
//	blink();

//let wakeable = new WakeableDigital({ pin: wakeup_pin });
//if (wakeable.read())
//	blink();

let wakeable = new WakeableDigital({ pin: wakeup_pin });
if ("digital" == wakeable.wakeupReason) {
	for (let i = 0; i < 10; ++i) {
		led.write(1);
		Timer.delay(50);
		led.write(0);
		Timer.delay(50);
	}
}

Timer.set(id => {
	led.write(0);
	led.close();
	Sleep.deep();
}, 3000);
