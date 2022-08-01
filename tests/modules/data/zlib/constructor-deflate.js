/*---
description: 
flags: [module]
---*/

import Deflate from "deflate";

let def;
try {
	def = new Deflate({});
}
catch {
}
finally {
	assert(def !== undefined, "not enough memory to test Deflate");
	def.close();
	def = undefined;
}

assert.throws(TypeError, () => Deflate(), "Deflate must be called as constructor");

def = new Deflate();
def.close();

assert.throws(RangeError, () => new Deflate({windowBits: -1}), "windowBits -1");
assert.throws(RangeError, () => new Deflate({windowBits: 100}), "windowBits 100");
assert.throws(TypeError, () => new Deflate({windowBits: Symbol()}), "windowBits Symbol()");

assert.throws(RangeError, () => new Deflate({level: -2}), "level -2");
assert.throws(RangeError, () => new Deflate({level: 100}), "level 100");
assert.throws(TypeError, () => new Deflate({level: Symbol()}), "level Symbol()");

assert.throws(RangeError, () => new Deflate({strategy: -1}), "strategy -1");
assert.throws(RangeError, () => new Deflate({strategy: 100}), "strategy 100");
assert.throws(TypeError, () => new Deflate({strategy: Symbol()}), "strategy Symbol()");
