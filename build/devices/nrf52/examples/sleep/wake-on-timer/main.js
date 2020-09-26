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

import {Sleep} from "sleep";
import Timer from "timer";

const led = new Host.LED;

// Blink LED upon wakeup
for (let i = 0; i < 10; ++i) {
	led.write(0);
	Timer.delay(50);
	led.write(1);
	Timer.delay(50);
}

Timer.set(() => {
	led.write(0);
	Sleep.wakeOnTimer(5000);
}, 3000);

