/*---
description: 
flags: [module]
---*/

import Time from "time";
import Timer from "timer";

assert.throws(SyntaxError, () => Time.delta(), "at least one argument requred");

checkDelay(4);
checkDelay(10);
checkDelay(100);

function checkDelay(delay) {
	const start = Time.ticks;
	Timer.delay(delay);
	const end = Time.ticks;
	let d0 = Time.delta(start);
	let d1 = Time.delta(start, end);
	d0 -= delay;
	d1 -= delay;
	assert((d0 >= -1) && (d0 <= 2), `d0 ${d0} incorrect for delay ${delay}`);
	assert((d1 >= -1) && (d1 <= 2), `d1 ${d1} incorrect for delay ${delay}`);
}
