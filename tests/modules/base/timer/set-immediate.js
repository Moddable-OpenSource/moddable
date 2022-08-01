/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

const start = Date.now();
Timer.set($DO(() => {
	const delta = (Date.now() - start);
	assert((delta >= -1) && (delta <= 3), `Timer.set immediate late`);
}));
