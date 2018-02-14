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
import Monitor from "monitor";

let led1 = new Digital({pin: 4, port: "gpioPortF", mode: Digital.Output});
let led2 = new Digital({pin: 5, port: "gpioPortF", mode: Digital.Output});

let monitor1 = new Monitor({pin: 6, port: "gpioPortF", mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge});
monitor1.onChanged = function() {
	trace("Button 0, ", this.read(), " times\n");
}
let monitor2 = new Monitor({pin: 7, port: "gpioPortF", mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge});
monitor2.onChanged = function() {
	trace("Button 1, ", this.read(), " times\n");
}

let count = 0;
Timer.repeat(() => {
	trace(`repeat ${++count} \n`);
	led1.write(~count & 1);
	led2.write(count & 1);
}, 200);

