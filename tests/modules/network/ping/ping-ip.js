/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";
import Net from "net";

await $NETWORK.connected;

const host = "test.moddable.com";
const id = 0;
const interval = 1000;

Net.resolve(host, (name, address) => {
	if (address) {
		new Ping({host: address, id, interval}, function(message, value, etc) {
			switch (message) {
				case Ping.error:
					$DONE(`Error: ${value}`);
					break;
				case Ping.response:
					$DONE();
					break;
			}
		});
	}
	else
		$DONE(`could not resolve "${name}"`);
});

$TESTMC.timeout(5_000);
