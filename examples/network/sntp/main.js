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

import SNTP from "sntp";
import Time from "time";
import Timer from "timer";

/*
	SNTP constructor takes an IP address. If this address used below is not responding, try another.
	To find a working SNTP server, try "ping pool.ntp.org".
	To programmatically resolve an SNTP server name to an IP address, use Net.resolve.
*/


new SNTP({address: "64.113.44.55"}, (message, value) => {
	switch (message) {
		case 1:
			trace(`Received SNTP time stamp ${value}.\n`);
			Time.set(value);
			break;

		case 2:
			trace("No response. Retrying.\n");
			break;

		case -1:
			trace("Failed.\n");
			break;
	}
});

Timer.repeat(id => trace(Date() + "\n"), 1000);
