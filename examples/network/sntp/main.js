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

import SNTP from "sntp";
import Time from "time";
import Timer from "timer";

const hosts = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"];

new SNTP({host: hosts.shift()}, function(message, value) {
	switch (message) {
		case SNTP.time:
			trace("Received time ", value, ".\n");
			Time.set(value);
			break;

		case SNTP.retry:
			trace("Retrying.\n");
			break;

		case SNTP.error:
			trace("Failed: ", value, "\n");
			if (hosts.length)
				return hosts.shift();
			break;
	}
});

Timer.repeat(id => trace(Date() + "\n"), 1000);
