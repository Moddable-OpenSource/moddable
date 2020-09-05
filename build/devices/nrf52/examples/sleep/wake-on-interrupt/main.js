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
	This application demonstrates how to use the Sleep object to trigger wakeup on an interrupt.
	The LIS3DH accelerometer is configured to generate an interrupt on motion.
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the application re-launches and blinks the LED if the motion interrupt was used to wakeup the device.

	https://devzone.nordicsemi.com/f/nordic-q-a/35408/lis2dh-int1-interrupt-for-wake-up/152144#152144
*/

import {Sleep, ResetReason} from "sleep";
import Timer from "timer";
import Digital from "pins/digital";
import {Sensor, Range, DataRate} from "lis3dh";
import config from "mc/config";

const led_pin = config.led1_pin;
const int_pin = config.lis3dh_int1_pin;
const ON = 1;
const OFF = 0;

// configure motion sensor
let sensor = new Sensor({
		range:Range.RANGE_2_G,
		rate:DataRate.DATARATE_10_HZ,
		interrupt: {
			polarity: 0,
			enable: 0x2a,
			threshold: 0x06,
			duration: 0x02
			}
		});

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
		
		// wakeup on interrupt pin
		Sleep.wakeOnInterrupt(int_pin);

		// turn off led while asleep
		Digital.write(led_pin, OFF);
		
		Sleep.deep();
	}
	--count;
}, 1000);

