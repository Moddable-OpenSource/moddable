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

import TM1637 from "embedded:peripheral/Display/TM1637";
import Timer from "timer";

const display = new TM1637({
	sensor: {
		io: device.io.Digital,
		data: device.i2c.default.data,
		clock: device.i2c.default.clock,
	},
	onError: error => trace(`${error}\n`)
});
display.configure({ brightness: 4 });

let state = 0;
const timer = Timer.repeat(() => {
	switch (state) {
		case 0:
			display.write("12:34");
			break;
		case 4:
			display.write("HELLO 1234");
			display.configure({ brightness: 7, rate: 200 });
			break;
		case 12:
			display.configure({ brightness: 2, rate: 100, direction: -1 });
			break;
		case 24:
			display.write("DONE");
			break;
		case 28:
			display.clear();
			display.close();
			Timer.clear(timer);
			break;
	}
	state += 1;
}, 1000);
