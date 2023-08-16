/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

import config from "mc/config";
import PulseCount from "pins/pulsecount";
import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";


const pulse = new PulseCount({
	signal: config.jogdial.signal,
	control: config.jogdial.control,
	onReadable() {
		trace(`Pulse ${this.read()}\n`);
	}
});

const button = new Monitor({
	pin: config.jogdial.button,
	mode: Digital.InputPullUp,
	edge: Monitor.Falling | Monitor.Rising,
});
button.onChanged = function() {
	if (this.read())
		trace(`Button Release\n`);
	else
		trace(`Button Push\n`);
}

const back = new Monitor({
	pin: 13,
	mode: Digital.InputPullUp,
	edge: Monitor.Falling | Monitor.Rising,
});
back.onChanged = function() {
	if (this.read())
		trace(`Back Release\n`);
	else
		trace(`Back Push\n`);
}
