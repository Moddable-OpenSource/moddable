/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";

await $NETWORK.connected;

const host = "marvell.com";
const id = 0;
const interval = 500;

let timeouts = 0;

new Ping({host, id, interval}, function(message, value, etc) {
	switch (message) {
		case Ping.error:
			$DONE(`Error: ${value}`);
			break;
		case Ping.response:
			$DONE(`no response expected`);
			break;
		case Ping.timeout:
			if (3 === ++timeouts)
				$DONE();
			break;
	}
});

$TESTMC.timeout(5_000);
