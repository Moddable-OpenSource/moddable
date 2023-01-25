/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";
import Net from "net";

await $NETWORK.connected;

Net.resolve("www.example.com", (name, address) => {
	if (!address) {
		$DONE("resolve failed");
		return;
	}

	const request = new Request({address, path: "/"});
	request.callback = function(message, value, etc) {
		if (Request.status === message) {
			$DONE()
			this.close();
		}
		else
			$DONE("unexpected message " + message)
	}
})

$TESTMC.timeout(5_000);
