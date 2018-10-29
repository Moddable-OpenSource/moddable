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

import MDNS from "mdns";
import {Request} from "http"

let mdns = new MDNS({});
mdns.monitor("_http._tcp", (service, instance) => {
	trace(`Found ${service}: "${instance.name}" @ ${instance.target} (${instance.address}:${instance.port})\n`);

	(new Request({address: instance.address, port: instance.port, path: "/"}).callback = function(message, value, etc) {
	 	if (2 == message)
			trace(`${value}: ${etc}\n`);	// HTTP header
		if (5 === message)
			trace("\n\n");					// end of request
		if (message < 0)
			trace("error \n\n");			// end of request with error
	});
});
