/*---
description: 
flags: [module]
---*/

function check(value) {
	return function() {
		const c = new Compartment({}, {
			foo: value
		});
	}
}

assert.throws(TypeError, check(null), "null");
assert.throws(TypeError, check(false), "boolean");
assert.throws(TypeError, check(0), "number");
assert.throws(TypeError, check(Symbol()), "symbol");
assert.throws(TypeError, check({}), "object");
assert.throws(TypeError, check([]), "array");
assert.throws(TypeError, check(new Proxy({}, {})), "proxy");
