/*---
description: 
flags: [async, module]
---*/

import Net from "net"
import {Request} from "http"
import SecureSocket from "securesocket";

await $NETWORK.connected;

const host = "www.google.com";

Net.resolve(host, (name, address) => {
	if (address)
		expectSuccess(address);
	else
		$DONE(`could not resolve "${name}"`);
});

let request;
function expectSuccess(address) {
	request = new Request({host, path: "/",
			port: 443, Socket: SecureSocket, secure: {verify: true}});
	request.callback = function(message, value, etc) {
		this.close();
		if (Request.status === message)
			expectFail(address)
		else
			$DONE("unexpected failure using host name");
	}
}

function expectFail(address) {
	request = new Request({host: address, path: "/",
			port: 443, Socket: SecureSocket, secure: {verify: true}});
	request.callback = function(message, value, etc) {
		this.close();
		if (Request.status === message)
			$DONE("unexpected success using resolved IP address");
		else
			expectSuccessNoVerify(address);
	}
}

function expectSuccessNoVerify(address) {
	request = new Request({host: address, path: "/",
			port: 443, Socket: SecureSocket, secure: {verify: false}});
	request.callback = function(message, value, etc) {
		this.close();
		if (Request.status === message)
			$DONE();
		else
			$DONE("unexpected failure with verify off");
	}
}

$TESTMC.timeout(10_000);
