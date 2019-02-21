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
import MCP23008 from "mcp23008";

export default function() {
	let mask = 0x88;

	Timer.repeat(() => {
		mask = ((mask << 1) | (mask >> 7)) & 0xFF;

		for (let i = 0; i < 8; i++)
			MCP23008.write(i, (mask & (1 << i)) ? 1 : 0);
	}, 50);
}
