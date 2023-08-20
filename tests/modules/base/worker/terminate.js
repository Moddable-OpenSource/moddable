/*---
description: 
flags: [module]
---*/

import Worker from "worker";

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

let worker = new Worker("testworker", minimumOptions);

assert.throws(SyntaxError, () => worker.terminate.call(new $TESTMC.HostObject, 0, 64), "terminate with non-worker this");

worker.terminate();
worker.terminate();		// safe to call more than once

assert.throws(SyntaxError, () => worker.postMessage(1), "postMessage after terminate");
