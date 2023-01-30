/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

function next() {
	let request = new Request({host: "www.example.com", path: "/",
			port: 443, Socket: SecureSocket, secure: {verify: true}});
	request.callback = function(message, value, etc) {
		if (message < 0)
			$DONE();
		else
			$DONE("unexpected success");
	}
}

let request = new Request({host: "www.example.com", path: "/",
		port: 443, Socket: SecureSocket, secure: {verify: false}});
request.callback = function(message, value, etc) {
	if (Request.status === message) {
		this.close();
		next();
	}
	else
		$DONE("unexpected failure");
}

$TESTMC.timeout(5_000);
