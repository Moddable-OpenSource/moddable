/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

assert.throws(SyntaxError, () => Timer.repeat(), "requires two arguments")
assert.throws(TypeError, () => Timer.repeat(function(){}, 1n), "can't make integer")

let start = Date.now();
let count = 0;
Timer.repeat(t => {
	let expected = ++count * 5;
	let delta = (Date.now() - start) - expected;
	if ((delta < -1) || (delta >= 3)) {
		Timer.clear(t);
		return $DONE("repeat off");
	}
	
	if (8 === count) {
		Timer.clear(t);
		$DONE();
	}
}, 5);
