/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const request = new Request({host: $NETWORK.invalidDomain, path: "/", response: String});
request.callback = function(message, value, etc) {
	if (message < 0)
		$DONE()
	else
		$DONE("unexpected message " + message);
}

$TESTMC.timeout(5_000);
