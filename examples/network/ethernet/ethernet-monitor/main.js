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
 
import Time from "time";
import Timer from "timer";
import SNTP from "sntp";
import SecureSocket from "securesocket";
import {Request} from "http";
import EthernetMonitor from "ethernet/connection";
new EthernetMonitor();


var count = 0;
var timeSet = 0;

function setSNTPTime() {
	const hosts = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org", "pool.ntp.org"];
	try {
		// Replace with a reliable NTP server, like pool.ntp.org
		trace(`setSNTPTime() Getting SNTP time\n`);
		let sntp = new SNTP({host: hosts.shift()}, function(message, value) {
			switch (message) {
				case SNTP.time:
					trace("setSNTPTime() Received Epoch time ", value, "\n");
					Time.set(value);
					let now = new Date(value * 1000);
					trace('setSNTPTime() Set time to ', now.toUTCString(),'\n');
					timeSet = 1;
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
		// The ESP32 date needs to be set for the SSL certificates to validate
		if (Date.now() > 1735689600000) {
			var request = new Request({	
				host: "timeapi.io", 
				path: "/api/Time/current/zone?timeZone=UTC", 
				response: String,
				Socket: SecureSocket,
				port: 443 }
			);
			request.callback = function(message, response) {
				if (message == Request.responseComplete) {
					let data = JSON.parse(response);
					trace(`getTime() The time is ${data.dateTime} GMT\n`);
					request.close();	// free the socket
				} else if (message == Request.error) {
					trace(`getTime() Request error\n`);
					request.close();	// free the socket
				}
			}
		} else {
			let date = new Date();
			trace ('Time is not set and SSL will fail. Date=',date, ' seconds=',date.getSeconds(),'\n');
		}
	}
	catch (e) {
		trace(`getTime() Error: ${e}\n`);
		debugger;
	}
}


// Now start a time to get the current time from the secure secrer.
Timer.repeat(() => {
	count++;
	//eval("trace('eval Running timer\\n');");
	var hours = count / 120.0;
	trace(`main-Running timer ${count} times, ${hours.toFixed(2)} hours\n`);
	if (ethernet.connected === true) {
		// If the time hasn't been set, set it now
		if (timeSet == 0) setSNTPTime(); 
		// We need the time to set be for SSL to work
		if (timeSet == 1) {
			trace(`main-Getting time\n`);
			getTime();
		}
	} else {
		trace(`Ethernet not connected\n`);
	}
}, 30000);