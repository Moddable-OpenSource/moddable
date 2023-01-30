/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";
import { URLSearchParams } from "url";

await $NETWORK.connected;

const body = new URLSearchParams([
	["Date", Date()],
	["Input", "This is no input!"]
]);

const request = new Request({host: "httpbin.org", path: "/post", method: "POST",
	headers: [
		'Content-Type', 'application/x-www-form-urlencoded;charset=UTF-8',
		"Date", Date(),
	],
	body: body.toString(),
	response: String,
});
request.callback = function(message, value, etc) {
	if (Request.responseFragment === message)
		$DONE("unexpected fragment");
	else if (Request.status === message) {
		if (200 !== value)
			$DONE("unexpected http result code " + value)
	}
	else if (Request.responseComplete == message) {
		if ("string" !== typeof value)
			$DONE("unexpected response type " + (typeof value))
		else {
			value = JSON.parse(value);
			if ((value.form.Date !== body.get("Date")) ||
				(value.form.Input !== body.get("Input")))
				$DONE("bad response");
			else
				$DONE();
		}
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(5_000);
