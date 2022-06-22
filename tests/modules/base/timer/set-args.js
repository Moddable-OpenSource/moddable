/*---
description: 
flags: [module]
---*/

import Timer from "timer";

assert.throws(SyntaxError, () => Timer.set(), "requires one argument")
assert.throws(TypeError, () => Timer.set(function(){}, 1n), "can't make integer")
assert.throws(TypeError, () => Timer.set(function(){}, 1, 1n), "can't make integer")
assert.throws(RangeError, () => Timer.set(function(){}, {
	[Symbol.toPrimitive]() {
		throw new RangeError;
	}
}), "can't make integer")

Timer.set(function() {});		// second argument optional
Timer.set(null, 1000);			// invalid callback error is on invocation, not creation
Timer.set(1, 1000);				// invalid callback error is on invocation, not creation
