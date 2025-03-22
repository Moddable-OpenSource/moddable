/*---
description: 
flags: [async, module]
---*/

import UDP from "embedded:io/socket/udp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const address = "127.0.0.1";
const port = 51515;
const testString = "This is a test!";
let u = new UDP({
	port,
	onReadable(count) {
		$DO(() => {
			const packet = this.read();
			assert.sameValue(typeof packet.port, "number", "port propty should be number");
			assert(packet.port != port, "port propty should not match receiver port");
			assert.sameValue(typeof packet.address, "string", "adddress propty should be string");
			assert.sameValue(packet.address, address, "packet address should be localhost");
			assert.sameValue(String.fromArrayBuffer(packet), testString, "incorrect packet content");
			this.close();
		})();
	}
});

let uu = new UDP({});
uu.write(address, port, ArrayBuffer.fromString(testString));
uu.close();

assert.throws(TypeError, () => new UDP({port: Symbol()}));
