/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const from = new TCP({
	address: await $NETWORK.resolve("www.example.com"),
	port: 80,
	format: "number",
	onReadable(bytes) {
		$DONE("this onReadable should never be called")
	},
	onWritable(bytes) {
		$DONE("this onWritable should never be called")
	},
	onError() {
		$DONE("this onError should never be called")
	}
});


new TCP({
	from,
	onWritable(bytes) {
		$DO(() => {
			assert.sameValue(typeof bytes, "number", "expect number");
			assert(bytes > 0, "expect at least one writable byte");
			assert.sameValue(this.format, "buffer", "expect format to be buffer");
			this.close();
		})();
	},
	onError() {
		$DONE("error");
	}
});

assert.throws(SyntaxError, () => new TCP({from}));
from.close();
from.close();
assert.throws(SyntaxError, () => from.read());

assert.throws(SyntaxError, () => new TCP({from: $TESTMC.HostObject}));
assert.throws(SyntaxError, () => new TCP({from: $TESTMC.HostObjectChunk}));
assert.throws(SyntaxError, () => new TCP({from: $TESTMC.HostBuffer}));
assert.throws(SyntaxError, () => new TCP({from: []}));
