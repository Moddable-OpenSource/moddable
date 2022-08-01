/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

let start = Date.now();
let count = 0;
Timer.set(t => {
	let expected = 10 + (count++ * 4);
	let delta = (Date.now() - start) - expected;
	if ((delta < -1) || (delta >= 3)) {
		Timer.clear(t);
		return $DONE("repeat off");
	}
	
	if (5 === count) {
		Timer.clear(t);
		$DONE();
	}
}, 10, 4);
