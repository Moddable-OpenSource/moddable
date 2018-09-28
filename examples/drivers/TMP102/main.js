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

import TMP102 from "tmp102";
import Timer from "timer";
import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

const sensor = new TMP102;
sensor.configure({
	alert: {
		above: sensor.sample().temperature + 1,
	},
	hz: 4,
});

let last, alert;
Timer.repeat(id => {
	let value = sensor.sample().temperature;
	if (value !== last) {
		trace(`Celsius temperature: ${value}\n`);
		last = value;
	}
}, 100);

(new Monitor({pin: 13, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling})).onChanged = function() {
	trace(`Alert: ${this.read()}\n`);
	sensor.configure({alert: {}});	// disable alerts
}
