/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const closers = [Request.status, Request.header, Request.headersComplete, Request.responseFragment, Request.responseComplete];

function next() {
	if (0 === closers.length)
		return $DONE();

	const close = closers.shift();
	const request = new Request({host: "www.example.com", path: "/"});
	request.callback = function(message, value, etc) {
		if (this.CLOSED)
			$DONE("unexpected callback after close");

		if (message === close) {
			this.close();
			this.CLOSED = true;
			next();
		}
		else if (Request.responseFragment === message)
			this.read(null);
	}
}
next();

$TESTMC.timeout(10_000);
