/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

const request = new Request({host: "8.8.8.8", path: "/",
		port: 443, Socket: SecureSocket, secure: {verify: true}});
request.callback = function(message, value, etc) {
	this.close();
	if (Request.status === message)
		$DONE();
	else
		$DONE("unexpected failure using ip address");
}

$TESTMC.timeout(5_000);
