/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

const address = await $NETWORK.resolve("pool.ntp.org");

$TESTMC.timeout(5_000);

const u = new UDP({
	onReadable(count) {
		$DO(() => {
			assert.sameValue(typeof count, "number", "expect number");
			assert(count > 0, "expect at least one packet");
			assert.sameValue(this.format, "buffer", "expect default format to be buffer");

			let result = this.read();
			assert(result instanceof ArrayBuffer, "expect ArrayBuffer");

			this.read();		// read possiblly recived packet
			this.read();		// read possiblly recived packet
			result = this.read();
			assert.sameValue(result, undefined, "read with no packet expected undefined");

			this.close();
		})();
	}
});

const sntpPacket = new Uint8Array(48);
sntpPacket[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);
u.write(address, 123, sntpPacket);
