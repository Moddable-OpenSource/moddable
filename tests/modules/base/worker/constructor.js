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

assert.throws(SyntaxError, () => new Worker, "Worker constructor requires 1 argument");
assert.throws(TypeError, () => new Worker(Symbol()), "Worker constructor rejects Symbol");
assert.throws(TypeError, () => Worker("testworker"), "Worker constructor called as function");

assert.throws(Error, () => new Worker("invalid module specifier", minimumOptions), "Worker constructor with invalid module specifier");
assert.throws(Error, () => new Worker(12, minimumOptions), "Worker constructor with invalid module specifier");

assert.throws(Error, () => new Worker("testworker", {allocation: 1024 * 1024 * 1024, stackCount: 64, slotCount: 64, keyCount: 7}), "Worker with 1 GB alocation should fail on microcontroller (deprecated property names)");
assert.throws(Error, () => new Worker("testworker", {...minimumOptions, static: 1024 * 1024 * 1024}), "Worker with 1 GB alocation should fail on microcontroller");
assert.throws(Error, () => new Worker("testthrowworker", minimumOptions), "Worker module throws");

new Worker("testworker", minimumOptions)

