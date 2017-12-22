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
import {MCP23008} from "MCP230XX";

export default function() {
	const leds = new MCP23008(/* defaults to 0x20! */);
	const io = new MCP23008({ address: 0x21 });
	let mask = 0x88;

	// These are buttons.
	io[0].mode(Digital.Input);
	io[1].mode(Digital.Input);
	io[2].mode(Digital.Input);

	// These are LEDs, writing to them will
	// set the direction to OUTPUT.
	io[3].write(0);
	io[4].write(0);
	io[5].write(0);

	Timer.repeat(() => {
		mask = ((mask << 1) | (mask >> 7)) & 0xFF;

		// Update the leds
		for (let i = 0; i < 8; i++) {
			leds[i].write(mask & (1 << i) ? 1 : 0);

			// Read the buttons and update the outputs
			if (i < 3) {
				io[i + 3].write(io[i].read() ? 0 : 1);
			}
		}
	}, 50);
}
