/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

let fragments = 0;

const request = new Request({host: "www.example.com", path: "/"});
request.callback = function(message, value, etc) {
	if (Request.responseFragment === message) {
		this.read(ArrayBuffer);
		fragments += 1;
	}
	else if (Request.responseComplete == message) {
		if (!fragments)
			$DONE("expected at least one fragment");
		else if (!value)
			$DONE();
		else
			$DONE("expected empty response");
	}
}

$TESTMC.timeout(5_000);
