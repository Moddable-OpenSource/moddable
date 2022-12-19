/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

let t = Timer.set(() => {$DONE("should never reach here");}, 1_000_000);

assert.throws(SyntaxError, () => Timer.schedule(), "no argument");
assert.throws(SyntaxError, () => Timer.clear(new $TESTMC.HostObject), "schedule arbitrary host object");
assert.throws(TypeError, () => Timer.schedule(t, 1n), "big int delay");
assert.throws(TypeError, () => Timer.schedule(t, Symbol()), "symbol delay");
assert.throws(TypeError, () => Timer.schedule(t, 1_000_000, 2n), "big int repeat");
assert.throws(TypeError, () => Timer.schedule(t, 1_000_000, Symbol()), "symbol repeat");

Timer.schedule(t, 10);
Timer.schedule(t);		// unschedule

let start = Date.now();
let count = 0;
const repeater = Timer.set(t => {
	let expected = count++ * 5;
	let delta = (Date.now() - start) - expected;
	if ((delta < -1) || (delta >= 3)) {
		Timer.clear(t);
		return $DONE("repeat off");
	}
	
	if (3 === count) {
		Timer.clear(t);
		$DONE();
	}
}, 1_000_000);
Timer.schedule(repeater, 0, 5);
