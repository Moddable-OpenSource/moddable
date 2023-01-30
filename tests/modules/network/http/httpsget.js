/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

let headers = 0;
let request = new Request({host: "www.mozilla.org", path: "/",
		port: 443, Socket: SecureSocket});
request.callback = function(message, value, etc)
{
	if (Request.header === message)
		headers++;
	else if (Request.responseFragment === message)
		this.read(null);
	else if (Request.responseComplete === message) {
		if (headers)
			$DONE();
		else
			$DONE("missing parts")
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(5_000);
