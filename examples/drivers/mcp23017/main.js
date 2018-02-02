/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
import {MCP23017} from "MCP230XX";

export default function() {
	const expander = new MCP23017({
		address: 0x20,
		inputs: 0b1111111100000000,
	});
	const led = expander[8];
	const button = expander[15];

	let ports = { a: 0x88, b: 0x00 };
	let indicator = 0;
	let delay = 120;
	let last = Date.now();

	// Set pins 0-15
	expander.write(0x0000);

	button.mode(Digital.Input);

	Timer.repeat(() => {
		// Set pins 0-7 (Or: port A)
		ports.a = ((ports.a << 1) | (ports.a >> 7)) & 0xFF;


		// Throttle button reading...
		let now = Date.now();

		if (now >= last + delay) {
			last = now;

			// Check if the button is pressed...
			if (expander.read() >> 15) {
				indicator ^= 1;
			}
		}
		// Set pins 8-15 (Or: port B)
		ports.b = (indicator << 8);

		expander.write(ports.b | ports.a);
	}, 50);
}
