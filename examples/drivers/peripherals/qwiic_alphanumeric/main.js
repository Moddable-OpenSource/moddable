/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import QwiicAlphanumericDisplay from "embedded:peripheral/Display/QwiicAlphanumeric";
import Timer from "timer";

trace("QwiicAlphanumericDisplay create.\n");
const display = new QwiicAlphanumericDisplay({
	sensor: {
		io: device.io.SMBus,
		data: device.i2c.default.data,
		clock: device.i2c.default.clock,
		hz: 400_000
	},
	addresses: [0x70, 0x71],		// add 0x71, 0x72… for chained units (left → right)
	onError: e => trace(e)
});

let state = 0;

let clanker = Timer.repeat(() => {
	switch (state) {
		case 0:
			display.write("Hello   ");
			break;
		case 4:
			display.write("This is a long scrolling string.");
			display.configure({ brightness: 8, rate: 200 });
			break;
		case 8:
			display.configure({ brightness: 6, rate: 100 });
			break;
		case 28:
			display.write("DONE");
			display.close();						// or use with {using} / Symbol.dispose
			Timer.clear(globalThis.clanker);
			break;
	}
	state += 1;
}, 1000);

//			display.setDisplay(true);				// show

