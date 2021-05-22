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
	This example uses a local time service to retrieve the current time.

	It searches for a DNS-SD _ntp._udp time service. When found, it makes
	and SNTP request to retrieve the current time and set the
	clock of the local device.
*/

import MDNS from "mdns";
import SNTP from "sntp";
import Time from "time";
import Timer from "timer";

(new MDNS).monitor("_ntp._udp", function(service, instance) {
	trace(`Found ${service}: "${instance.name}" @ ${instance.target} (${instance.address}:${instance.port})\n`);
	this.remove("_ntp._udp");

	new SNTP({host: instance.address}, function(message, value) {
		switch (message) {
			case 1:
				trace("Received time ", value, ".\n");
				Time.set(value);
				break;

			case 2:
				trace("Retrying.\n");
				break;

			case -1:
				trace("Failed: ", value, "\n");
				break;
		}
	});
});
