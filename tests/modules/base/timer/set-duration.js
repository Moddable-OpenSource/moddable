/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

const expected = 100;
const start = Date.now();
Timer.set($DO(() => {
	let delta = (Date.now() - start) - expected;
	assert((delta >= -1) && (delta < 3), `Timer.set of ${expected} off by ${delta}`);
}), expected);
