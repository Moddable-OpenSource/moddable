/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

const host = "ecmatc53.github.io";

const request = new Request({host, path: "/",
		port: 443, Socket: SecureSocket, secure: {verify: true}});
request.callback = function(message, value, etc) {
	this.close();
	if (Request.status === message)
		$DONE();
	else
		$DONE("failure with wildcard SAN");
}
