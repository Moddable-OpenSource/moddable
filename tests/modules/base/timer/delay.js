/*---
description: 
flags: [module]
---*/

import Timer from "timer";

function doDelay(ms, expected = ms) {
	const start = Date.now();
	Timer.delay(ms);
	const delta = (Date.now() - start) - expected;
	assert((delta >= -1) && (delta < 3), `delay of ${ms} off by ${delta}`);
}

doDelay(123);
doDelay(1);
doDelay(0);
doDelay(true, 1);

assert.throws(SyntaxError, () => Timer.delay(), "requires one argument")
assert.throws(RangeError, () => Timer.delay("error"), "Timer.delay rejects NaN")
assert.throws(RangeError, () => Timer.delay(undefined), "Timer.delay rejects undefined")
assert.throws(RangeError, () => Timer.delay(-1), "Timer.delay rejects negative")
assert.throws(RangeError, () => Timer.delay(Infinity), "Timer.delay rejects Infinity")
assert.throws(RangeError, () => Timer.delay(2 ** 31), "Timer.delay rejects more than 2^31 - 1")
assert.throws(TypeError, () => Timer.delay(Symbol()), "Timer.delay rejects symbol")
assert.throws(TypeError, () => Timer.delay(5n), "Timer.delay rejects BigInt")
