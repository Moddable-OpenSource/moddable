/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const request = new Request({host: "www.example.com", path: "/", response: ArrayBuffer});
request.callback = function(message, value, etc) {
	if (Request.responseFragment === message)
		$DONE("unexpected fragment");
	else if (Request.responseComplete == message) {
		if (value instanceof ArrayBuffer)
			$DONE();
		else
			$DONE("response not ArrayBuffer");
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(5_000);
