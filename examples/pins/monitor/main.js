/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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
import Monitor from "pins/digital/monitor";
import config from "mc/config";

let led1 = null;
let led2 = null;

if (undefined != config.led1_pin)
	if (config.led1_port)
		led1 = new Digital({pin: config.led1_pin, port: config.led1_port, mode: Digital.Output});
	else
		led1 = new Digital({pin: config.led1_pin, mode: Digital.Output});
if (undefined != config.led2_pin)
	if (config.led2_port)
		led2 = new Digital({pin: config.led2_pin, port: config.led2_port, mode: Digital.Output});
	else
		led2 = new Digital({pin: config.led2_pin, mode: Digital.Output});

let monitor1 = null;
let monitor2 = null;

if (undefined != config.button1_pin) {
	if (config.button1_port)
		monitor1 = new Monitor({pin: config.button1_pin, port: config.button1_port, mode: Digital.InputPullUp, edge: Monitor.Falling | Monitor.Rising});
	else
		monitor1 = new Monitor({pin: config.button1_pin, mode: Digital.InputPullUp, edge: Monitor.Falling | Monitor.Rising});
	monitor1.onChanged = function() {
		trace(`Button 1: state: ${this.read()}, changed ${this.rises + this.falls} times\n`);
	}
}

if (undefined != config.button2_pin) {
	if (config.button2_port)
		monitor2 = new Monitor({pin: config.button2_pin, port: config.button2_port, mode: Digital.InputPullUp, edge: Monitor.Falling | Monitor.Rising});
	else
		monitor2 = new Monitor({pin: config.button2_pin, mode: Digital.InputPullUp, edge: Monitor.Falling | Monitor.Rising});
	monitor2.onChanged = function() {
		trace(`Button 2: state: ${this.read()}, changed ${this.rises + this.falls} times\n`);
	}
}

let count = 0;
Timer.repeat(() => {
	trace(`repeat ${++count} \n`);
	if (led1)
		led1.write(~count & 1);
	if (led2)
		led2.write(count & 1);
}, 500);

