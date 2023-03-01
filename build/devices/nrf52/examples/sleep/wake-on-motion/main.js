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
	This application demonstrates how to use the Digital object to trigger wakeup on an interrupt.
	The LIS3DH accelerometer is configured to generate an interrupt on motion.
	The application turns on the LED while running and turns off the LED when asleep.
	Upon wakeup, the application re-launches and blinks the LED if the motion interrupt was used to wakeup the device.

	https://devzone.nordicsemi.com/f/nordic-q-a/35408/lis2dh-int1-interrupt-for-wake-up/152144#152144
*/

import Digital from "pins/digital";
import {Sleep} from "sleep";
import Timer from "timer";
import {Range, DataRate} from "lis3dh";
import config from "mc/config";

const wakeup_pin = config.lis3dh_int1_pin;
const led = new Host.LED.Default;

// configure accelerometer to generate an interrupt on the wakeup pin
let sensor = new Host.Accelerometer({
	range:Range.RANGE_2_G,
	rate:DataRate.DATARATE_10_HZ,
	interrupt: {
		polarity: 0,
		enable: 0x2a,
		threshold: 0x06,
		duration: 0x02
	}
});

led.write(1);

let digital = new Digital({
	pin: wakeup_pin,
	mode: Digital.InputPullUp,
	wakeEdge: Digital.WakeOnFall,
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

