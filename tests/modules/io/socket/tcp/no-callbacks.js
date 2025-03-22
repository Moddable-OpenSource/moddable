/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";
import Timer from "timer";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const host = "www.example.com";
let t = new TCP({
	address: await $NETWORK.resolve(host),
	port: 80
});
assert.sameValue(t.read(Uint8Array.of(1)), undefined, "read before connected");

let wrote = false;
Timer.repeat(id => {
	try {
		const headers = [
			`GET / HTTP/1.1`,
			`Host: ${host}`,
			"Connection: close",
			"",
			"",
		].join("\r\n")
		t.write(ArrayBuffer.fromString(headers));

		wrote = true;
		Timer.clear(id);
	}
	catch {
	}
}, 50);

Timer.repeat(id => {
	try {
		const buffer = t.read();
		if (!buffer) return;

		if (!wrote)
			$DONE("bytes available to read before write");

		assert(buffer.byteLength >= 8, "expected at least 8 bytes");
		const bytes = new Uint8Array(buffer);
		assert.sameValue(String.fromArrayBuffer(bytes.buffer).slice(0, 8), "HTTP/1.1");

		t.close();
		Timer.clear(id);
		$DONE();
	}
	catch (e) {
		$DONE("unexpected: " + e);
	}
}, 50);
