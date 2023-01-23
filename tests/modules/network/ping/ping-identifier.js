/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";

await $NETWORK.connected;

const host = "test.moddable.com";
const id = 1234;
const interval = 1000;

// noise
new Ping({host, id: id + 1, interval: 200}, function(message, value, etc) {
});

// test
new Ping({host, id, interval}, function(message, value, etc) {
	switch (message) {
		case Ping.error:
			$DONE(`Error: ${value}`);
			break;
		case Ping.response:
			if (etc.identifier !== id)
				$DONE(`unexpected identifier: ${etc.identifier}`);
			else
				$DONE();
			break;
	}
});

$TESTMC.timeout(5_000);
