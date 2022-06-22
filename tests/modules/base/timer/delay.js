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
doDelay("error", 0);

assert.throws(SyntaxError, () => Timer.delay(), "requires one argument")
assert.throws(TypeError, () => Timer.delay(Symbol()), "Timer.delay rejects symbol")
assert.throws(TypeError, () => Timer.delay(5n), "Timer.delay rejects BigInt")
