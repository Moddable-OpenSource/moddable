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
/*
	This example implements a local time service on the local network. This allows devices on
	the local network to retrieve the current time without having to access the internet
	and without having to hardcode a list of ntp servers.

	The DNS-SD time service is advertised as time _ntp._udp, the same as the ntpd in Linux.
 	Additionally, the service puts the current UTC tiem as a string in the TXT record.
	This is updaed once a minute. The value is for debugging purposes; use the NTP
	service itself to retrieve the current time.

	The NTP server itself is trivial, it only fills in the header byte and current time.

	The ntpclient example uses the _ntp._udp service to retrieve the current time.
*/

import Timer from "timer";
import MDNS from "mdns";
import {Socket} from "socket";

function timeInMinutes() {
	let now = new Date;
	now.setUTCSeconds(0);
	now.setUTCMilliseconds(0);
	return now.valueOf();
}

function millisecondsToNextMinute() {
	let now = new Date;
	return (60 * 1000) - ((now.getUTCSeconds() * 1000) + now.getUTCMilliseconds());
}

class SNTP extends Socket {
	constructor() {
		super({kind: "UDP", port: 123});
	}
	callback(message, value, address, port) {
		if (2 !== message)
			return;

		const packet = new DataView(new ArrayBuffer(48));
		packet.setUint8(0, 0x24);
		packet.setUint32(40, ((Date.now() / 1000) | 0) + 2208988800);
		this.write(address, port, packet.buffer);
	}
}

const timeService = {name: "ntp", protocol: "udp", port: 123, txt: {}};

const mdns = new MDNS({hostName: "time"}, function(message, value) {
	if ((MDNS.hostName === message) && value) {
		timeService.txt.now = timeInMinutes();
		this.add(timeService);

		Timer.set(function() {
			timeService.txt.now = timeInMinutes();
			mdns.update(timeService);
		}, millisecondsToNextMinute(), 60 * 1000);
	}
});

new SNTP;
