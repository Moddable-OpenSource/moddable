/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

const host = "httpbin.org";

const request = new Request({host, path: "/encoding/utf8",
		port: 443, Socket: SecureSocket, secure: {verify: true}});
request.callback = function(message, value, etc) {
	this.close();
	if (Request.status === message)
		$DONE();
	else
		$DONE("SAN with leaf not last in server provided certificate list");
}
