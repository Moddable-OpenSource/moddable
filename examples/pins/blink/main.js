/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import Timer from "timer";
import Digital from "pins/digital";

// blinks the two LEDs on ESP8266 NodeMCU boards
// blinks only blue LED on ESP32 Node<MCU boards (pin 2).
// pi uses 25

let count = 0;
let led = 0;
const ledPin = 25;
Timer.repeat(() => {
	switch (count++ % 5) {
		case 0: Digital.write(ledPin, 1); break;
		case 1: break;
		case 2: break;
		case 3: Digital.write(ledPin, 0); break;
		case 4: break;
	}
}, 200);

