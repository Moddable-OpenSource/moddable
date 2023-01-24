/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const body = "This is no data!";

const request = new Request({host: "httpbin.org", path: "/put", method: "PUT",
	body,
	response: String,
});
request.callback = function(message, value, etc) {
	if (Request.responseFragment === message)
		$DONE("unexpected fragment");
	else if (Request.status === message) {
		if (200 !== value)
			$DONE("unexpected http result code " + value)
	}
	else if (Request.responseComplete == message) {
		if ("string" !== typeof value)
			$DONE("unexpected response type " + (typeof value))
		else {
			value = JSON.parse(value);
			if (value.data === body)
				$DONE();
			else
				$DONE("bad response");
		}
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(5_000);
