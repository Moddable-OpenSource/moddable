/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

let headers = 0, fragments = 0;
let request = new Request({host: "www.cloudflare.com", path: "/",
		port: 443, Socket: SecureSocket, secure: {protocolVersion: 0x0303}});
request.callback = function(message, value, etc)
{
	if (Request.header === message)
		headers++;
	else if (Request.responseFragment == message)
		fragments++;
	else if (Request.responseComplete === message) {
		if (headers && fragments)
			$DONE();
		else
			$DONE("missing parts")
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(8_000);
