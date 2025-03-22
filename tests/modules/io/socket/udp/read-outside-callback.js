/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

const address = await $NETWORK.resolve("pool.ntp.org");

$TESTMC.timeout(5_000);

const {promise: p, resolve, reject} = Promise.withResolvers();
let didOnReadable = false;

const u = new UDP({
	onReadable(count) {
		if (didOnReadable)
			return;
		resolve(count);
	}
});

assert.sameValue(u.read(), undefined, "expect undefined before packet available");

const sntpPacket = new Uint8Array(48);
sntpPacket[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);

assert(await p >= 1, "expected at least 1 packet");

assert(u.read() instanceof ArrayBuffer, "expect ArrayBuffer");

$DONE();
