/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

let u = new UDP({
});
u.close();

u = new UDP({
	onReadable(count) {
	}
});
u.close();

u = new UDP({
	port: 1234
});
u.close();

u = new UDP({
	port: 1234,
	onReadable(count) {
	}
});
u.close();

$DONE();

