/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";

await $NETWORK.connected;

const host = "test.moddable.com";
const id = 0;
const interval = 1000;

new Ping({host, id, interval}, function(message, value, etc) {
	switch (message) {
		case Ping.error:
			$DONE(`Error: ${value}`);
			break;
		case Ping.response:
			$DONE();
			break;
	}
});

$TESTMC.timeout(5_000);
