/*---
description: 
flags: [onlyStrict]
---*/

function test(options) {
	return (new Compartment(undefined, undefined, options)).toString();
}

assert.sameValue((new Compartment()).toString(), "[object Compartment]", "no options");
assert.sameValue(test(undefined), "[object Compartment]", "undefined");
assert.sameValue(test(null), "[object Compartment]", "null");
assert.sameValue(test(false), "[object Compartment]", "boolean");
assert.sameValue(test(0), "[object Compartment]", "number");
assert.sameValue(test(""), "[object Compartment]", "string");
assert.sameValue(test(Symbol()), "[object Compartment]", "symbol");
assert.sameValue(test({}), "[object Compartment]", "object");
assert.sameValue(test([]), "[object Compartment]", "array");
