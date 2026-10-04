/*---
description: 
flags: [module,async]
---*/

import {SharedWorker} from "worker";

const minimumOptions = {
	static: 8192,
	heap: {
		initial: 64,
		incremental: 64
	},
	chunk: {
		initial: 1024,
		incremental: 512
	},
	stack: 64,
	keys: {
		initial: 1
	}
};

assert.throws(SyntaxError, () => new SharedWorker, "SharedWorker constructor requires 1 argument");
assert.throws(TypeError, () => new SharedWorker(Symbol()), "SharedWorker constructor rejects Symbol");
assert.throws(TypeError, () => SharedWorker("testsharedworker"), "SharedWorker constructor called as function");

assert.throws(Error, () => new SharedWorker("invalid module specifier", minimumOptions), "SharedWorker constructor with invalid module specifier");
assert.throws(Error, () => new SharedWorker(12, minimumOptions), "SharedWorker constructor with invalid module specifier");

assert.throws(Error, () => new SharedWorker("testsharedworker", {...minimumOptions, static: 1024 * 1024 * 1024}), "SharedWorker with 1 GB alocation should fail on microcontroller");
assert.throws(Error, () => new SharedWorker("testthrowworker", minimumOptions), "SharedWorker module throws");

const count = 4;
for (let i = 0; i < count; i++) {
	const s = new SharedWorker("testsharedworker", minimumOptions);
	s.port.onmessage = function(message) {
		if ((i + 1) !== message.connectedCount)
			$DONE(`unexpected connectionCount on SharedWorker ${i}`);
		if (count === (i + 1))
			$DONE();
	}
}
