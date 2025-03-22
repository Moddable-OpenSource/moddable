/*---
description: 
flags: [async, module]
---*/

import {Request} from "http"
import SecureSocket from "securesocket";
import Resource from "Resource";

await $NETWORK.connected;

const host = "www.yahoo.com";

function next() {
	let request = new Request({host, path: "/",
			port: 443, Socket: SecureSocket, secure: {verify: true}});
	request.callback = function(message, value, etc) {
		this.close();
		if (message < 0)
			$DONE();
		else {
			if (Resource.exists("ca107.der"))
				$DONE(`didn't expect root certificate for ${host} to be found`);
			$DONE("unexpected success");
		}
	}
}

let request = new Request({host, path: "/",
		port: 443, Socket: SecureSocket, secure: {verify: false}});
request.callback = function(message, value, etc) {
	this.close();
	if (Request.status === message)
		next();
	else
		$DONE("unexpected failure");
}

$TESTMC.timeout(5_000);
