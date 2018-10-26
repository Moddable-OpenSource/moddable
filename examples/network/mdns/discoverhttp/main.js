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
