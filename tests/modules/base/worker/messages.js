/*---
description: 
flags: [module, async]
---*/

import Worker from "worker";

const minimumOptions = {
	allocation: 8192,
	stackCount: 64,
	slotCount: 64,
	keyCount: 7
};

const messages = [
	1,
	1n,
	"1",
	true,
	[1],
	{one: 1},
	new Date(1),
	Uint8Array.of(1).buffer,
	Uint32Array.of(1)
];

const worker = new Worker("testworker", minimumOptions);

assert.throws(SyntaxError, () => worker.postMessage(), "postMessage requires 1 argument");
assert.throws(SyntaxError, () => worker.postMessage.call(new $TESTMC.HostObject, 0, 64), "postMessage with non-worker this");
assert.throws(TypeError, () => worker.postMessage({host: new $TESTMC.HostObject}), "postMessage rejects host objects");

let index = 0;
worker.postMessage(messages[index]);

worker.onmessage = function(reply) {
	const expected = messages[index], actual = reply.value;
	try {
		if (actual !== expected) {
			if (4 === index) {
				assert(Array.isArray(actual), "expected array");
				assert.sameValue(1, actual.length, "expected array length 1");
				assert.sameValue(1, actual[0], "expected array index 0 to be 1");
			}
			else if (5 === index) {
				assert.sameValue(typeof actual, "object", "expected object");
				assert.sameValue(Object.keys(actual).length, 1, "expected one key");
				assert.sameValue(actual.one, 1, "expected property one to be 1");
			}
			else if (6 === index) {
				assert(actual instanceof Date, "expected date instance");
				assert.sameValue(actual.valueOf(), 1, "expected date valueOf to be 1");
			}
			else if (7 === index) {
				assert(actual instanceof ArrayBuffer, "expected ArrayBuffer instance");
				assert.sameValue(actual.byteLength, 1, "expected ArrayBuffer.byteLength 1");
				assert.sameValue((new Uint8Array(actual))[0], 1, "expected buffer[0] to be 1");
			}
			else if (8 === index) {
				assert(actual instanceof Uint32Array, "expected Uint32Array instance");
				assert.sameValue(actual.length, 1, "expected Uint32Array.length 1");
				assert.sameValue(actual[0], 1, "expected buffer[0] to be 1");
			}
			else
				throw new Error("unexpected");
		}
	}
	catch (e) {
		$DONE(e);
		return;
	}
	
	index += 1;
	if (index === messages.length) {
		$DONE();
		return 
	}

	worker.postMessage(messages[index]);
}
