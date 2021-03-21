/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import Digital from "pins/digital";
import config from "mc/config";

// blinks the two LEDs on ESP8266 NodeMCU boards
// blinks one led on other boards

let count = 0;
Timer.repeat(() => {
	trace(`repeat ${++count} \n`);
	Digital.write(config.led.pin, ~count & 1);
	if ( config.led.pin2 ) Digital.write(config.led.pin2, count & 1);
}, 200);
