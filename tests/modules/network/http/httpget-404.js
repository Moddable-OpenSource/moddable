/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const request = new Request({host: "www.example.com", path: "/test_not__found", response: String});
request.callback = function(message, value, etc) {
	if (Request.status === message) {
		if (404 === value) {
			$DONE()
			this.close();
		}
		else
			$DONE("unexpected http result code " + value)
	}
	else
		$DONE("unexpected message " + message);
}

$TESTMC.timeout(5_000);
