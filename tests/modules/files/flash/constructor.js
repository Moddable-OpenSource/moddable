/*---
description: 
flags: [module]
---*/

import Flash from "flash";

assert.throws(SyntaxError, () => new Flash, "Flash constructor requires 1 argument");
assert.throws(Error, () => new Flash(1n), "Flash constructor rejects BigInt");
assert.throws(TypeError, () => new Flash(Symbol()), "Flash constructor rejects Symbol");
assert.throws(TypeError, () => Flash($TESTMC.config.flashParition), "Flash constructor called as function");

let f = new Flash($TESTMC.config.flashParition);
let {byteLength, blockSize} = f;

f = new Flash({
	[Symbol.toPrimitive]() {
		return $TESTMC.config.flashParition;
	}
});
assert.sameValue(byteLength, f.byteLength, "same partition (same byteLength)s");
assert.sameValue(blockSize, f.blockSize, "same partition (same byteLength)s");

assert.throws(Error, () => new Flash("1245678"), "Flash constructor rejects unknown partition");
