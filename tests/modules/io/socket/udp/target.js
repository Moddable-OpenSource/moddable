/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

const address = await $NETWORK.resolve("pool.ntp.org");

$TESTMC.timeout(5_000);

const target = {};

const u = new UDP({
	target,
	onReadable(count) {
		$DO(() => {
			assert.sameValue(this.target, target, "bad target in callback");
			this.close();
		})();
	}
});

assert.sameValue(u.target, target, "bad target after instantiation");

const sntpPacket = new Uint8Array(48);
sntpPacket[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);
