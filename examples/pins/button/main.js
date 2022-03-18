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

// traces to console when FLASH button on ESP8266 NodeMCU boards is pressed.
// traces to console when IO0 button on ESP32 NodeMCU boards is pressed.

const BUTTON_PIN = 0;

trace(`Using pin ${BUTTON_PIN} for button.`);

const button = new Digital(BUTTON_PIN, Digital.InputPullUp);
let previous = 0;
Timer.repeat(() => {
	const current = button.read();
	if (current !== previous) {
		if (current)
			trace("button was pressed\n");
		previous = current;
	}
}, 100);
