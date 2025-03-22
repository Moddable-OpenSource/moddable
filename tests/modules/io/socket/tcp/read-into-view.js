/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const host = "www.example.com";
let wrote = false;
new TCP({
	address: await $NETWORK.resolve(host),
	port: 80,
	onWritable(bytes) {
		if (wrote)
			return;
		wrote = true;

		const headers = [
			`GET / HTTP/1.1`,
			`Host: ${host}`,
			"Connection: close",
			"",
			"",
		].join("\r\n")
		this.write(ArrayBuffer.fromString(headers));
	},
	onReadable(bytes) {
		$DO(() => {
			assert(bytes > 8, "test needs at least 8 bytes");
			const buffer = new ArrayBuffer(512);
			const view = new Uint8Array(buffer, 256, 8);
			let result = this.read(view)
			assert.sameValue(result, 8, "should read 8 bytes");
			let str = String.fromArrayBuffer(view.slice().buffer);
			assert.sameValue(str, "HTTP/1.1", "unexpected read result " + str);
			
			const all = new Uint8Array(buffer);
			for (let i = 0; i < 256; i++)
				assert.sameValue(all[i], 0, "read wrote outside range");

			for (let i = 256 + 8; i < 512; i++)
				assert.sameValue(all[i], 0, "read wrote outside range");

			this.close();
		})();
	},
	onError() {
		$DONE("error");
	}
});
