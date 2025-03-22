/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";
import Timer from "timer";

await $NETWORK.connected;

const address = await $NETWORK.resolve("pool.ntp.org");

$TESTMC.timeout(5_000);

const u = new UDP({});

assert.sameValue(u.read(), undefined, "expect undefined before packet available");

const sntpPacket = new Uint8Array(48);
sntpPacket[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);

Timer.repeat(id => {
	try {
		let buffer = u.read();
		if (undefined === buffer) return;

		assert(buffer instanceof ArrayBuffer, "expect ArrayBuffer");

		u.close();
		Timer.clear(id);
		$DONE();
	}
	catch (e) {
		$DONE("unexpected: " + e);
	}
}, 50);
