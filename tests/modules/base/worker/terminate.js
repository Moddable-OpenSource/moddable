/*---
description: 
flags: [module]
---*/

import Worker from "worker";

const minimumOptions = {
	allocation: 8192,
	stackCount: 64,
	slotCount: 64,
	keyCount: 7
};

let worker = new Worker("testworker", minimumOptions);

assert.throws(SyntaxError, () => worker.terminate.call(new $TESTMC.HostObject, 0, 64), "terminate with non-worker this");

worker.terminate();
worker.terminate();		// safe to call more than once

assert.throws(SyntaxError, () => worker.postMessage(1), "postMessage after terminate");
