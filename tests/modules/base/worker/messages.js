/*---
description: 
flags: [module, async]
---*/

import Worker from "worker";
import minimumOptions from "./minimumOptions_FIXTURE.js"

const messages = [
	1,
	1n,
	"1",
	true,
	[1],
	{one: 1},
	new Date(1),
	Uint8Array.of(1).buffer,
	Uint32Array.of(1),
	new Map([[1, "one"]]),
	new Set([1, "two", 3n]),
	1_000_000_000_000_000_000_000_000_000_000_000n
];

const worker = new Worker("testworker", minimumOptions);

assert.throws(SyntaxError, () => worker.postMessage(), "postMessage requires 1 argument");
assert.throws(SyntaxError, () => worker.postMessage.call(new $TESTMC.HostObject, 0, 64), "postMessage with non-worker this");
assert.throws(Error, () => worker.postMessage({host: new $TESTMC.HostObject}), "postMessage rejects host objects");

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
			else if (9 === index) {
				assert(actual instanceof Map, "expected Map instance");
				assert.sameValue(actual.size, 1, "expected Map.size 1");
				assert.sameValue(actual.get(1), "one", "expected get(1) to be 'one'");
			}
			else if (10 === index) {
				assert(actual instanceof Set, "expected Set instance");
				assert.sameValue(actual.size, 3, "expected Set.size 1");
				assert.sameValue(actual.has(1), true, "expected has(1) to be true");
				assert.sameValue(actual.has("two"), true, "expected has('two') to be true");
				assert.sameValue(actual.has(3n), true, "expected has(3n) to be true");
			}
			else if (11 === index) {
				assert.sameValue(typeof actual, "bigint", "expected bigint");
				assert.sameValue(actual, messages[11], "expected correct BigInt value");
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
