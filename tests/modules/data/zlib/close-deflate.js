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

def = new Deflate();

assert.throws(SyntaxError, () => def.close.call(new $TESTMC.HostObjectChunk), "close invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => def.close.call(new $TESTMC.HostObject), "close invalid this - HostObject");
assert.throws(SyntaxError, () => def.close.call(new $TESTMC.HostBuffer), "close invalid this - HostBuffer");

def.close();
def.close();
