/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";

await $NETWORK.connected;

const host = $NETWORK.invalidDomain;
const id = 0;
const interval = 500;

new Ping({host, id, interval}, function(message, value, etc) {
	switch (message) {
		case Ping.error:
			$DONE();
			break;
		case Ping.response:
		case Ping.timeout:
			$DONE("unexpected callback");
			break;
	}
});

$TESTMC.timeout(5_000);
