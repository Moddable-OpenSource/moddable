/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import {Request} from "http";
import EthernetMonitor from "ethernet/connection";

new EthernetMonitor();

function getTime() {
	let request = new Request({	host: "time.jsontest.com", path: "/", response: String});
	request.callback = function(message, value) {
		if (Request.responseComplete === message) {
			value = JSON.parse(value, ["time"]);
			trace(`The time is ${value.time} GMT\n`);
		}
	}
}

Timer.repeat(() => {
	if (ethernet.connected === true) {
		getTime();
	}
}, 5000);