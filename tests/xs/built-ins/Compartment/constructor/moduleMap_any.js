/*---
description: 
flags: [onlyStrict]
---*/

function test(moduleMap) {
	return (new Compartment(undefined, moduleMap)).toString();
}

assert.sameValue((new Compartment()).toString(), "[object Compartment]", "no moduleMap");
assert.sameValue(test(undefined), "[object Compartment]", "undefined");
assert.sameValue(test(null), "[object Compartment]", "null");
assert.sameValue(test(false), "[object Compartment]", "boolean");
assert.sameValue(test(0), "[object Compartment]", "number");
assert.sameValue(test(""), "[object Compartment]", "string");
assert.sameValue(test(Symbol()), "[object Compartment]", "symbol");
assert.sameValue(test({}), "[object Compartment]", "object");
assert.sameValue(test([]), "[object Compartment]", "array");
