/*
 * Copyright (c) 2016-2025 Moddable Tech, Inc.
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
 
import Time from "time";
import Timer from "timer";
import SNTP from "sntp";
import SecureSocket from "securesocket";
import {Request} from "http";
import EthernetMonitor from "ethernet/connection";


// Start the Ethernet Monitor
const monitor = new EthernetMonitor;

let count = 0;
let timeSet = 0;

function setSNTPTime() {
	const hosts = ["time.google.com", "time.cloudflare.com", "pool.ntp.org"];
	try {
		trace(`setSNTPTime() Getting SNTP time\n`);
		const sntp = new SNTP({host: hosts.shift()}, function(message, value) {
			switch (message) {
				case SNTP.time:
					trace("setSNTPTime() Received Epoch time ", value, "\n");
					Time.set(value);
					const now = new Date;
					trace('setSNTPTime() Set time to ', now.toUTCString(),'\n');
					timeSet = 1;
					Timer.schedule(timer, 0, 30_000);		// move to next step
					break;

				case SNTP.retry:
					trace("setSNTPTime() Retrying.\n");
					break;

				case SNTP.error:
					trace("setSNTPTime() Failed: ", value, "\n");
					if (hosts.length)
						return hosts.shift();
					break;
			}
		});
	}
	catch (e) {
		trace(`setSNTPTime() Error: ${e}\n`);
		debugger;
	}
}

function getTime() {
	try {
		// The ESP32 date needs to be set for the TLS certificates to validate
		const host = "timeapi.io";
		if (Date.now() < 1735689600_000)
			throw new Error `getTime() Requesting time from the ${host} server`;
		const request = new Request({	
			host: host, 
			path: "/api/Time/current/zone?timeZone=UTC", 
			response: String,
			Socket: SecureSocket,
			port: 443,
			secure: {
				protocolVersion: 0x303
			}
		});
		request.callback = function(message, response) {
			if (message == Request.responseComplete) {
				let data = JSON.parse(response);
				trace(`getTime() The ${host} time is ${data.dateTime} GMT\n`);
				request.close();	// free the socket
			} else if (message == Request.error) {
				trace(`getTime() ${host} request error\n`);
				request.close();	// free the socket
			}
		}
	}
	catch (e) {
		trace(`getTime() Error: ${e}\n`);
		debugger;
	}
}

// Start a timer to periodically get the current time from the secure server.
const timer = Timer.set(() => {
	const hours = ++count / 120;
	trace(`main-Running timer ${count} times, ${hours.toFixed(2)} hours\n`);
	if (!ethernet.connected)
		trace(`Ethernet not connected\n`);
	else if (timeSet === 0)
		setSNTPTime(); 	// If the time hasn't been set, set it now
	else
		getTime();			// Now that time is set, we can use TLS

}, 0, 30_000);
