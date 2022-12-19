/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

Timer.clear(Timer.set(() => {}, 1));
assert.throws(SyntaxError, () => Timer.clear(), "no argument");
assert.throws(SyntaxError, () => Timer.clear(new $TESTMC.HostObject), "clear arbitrary host object");

Timer.clear(undefined); 
Timer.clear(null); 

const t = Timer.set(() => {}, 1);
Timer.clear(t);
assert.throws(SyntaxError, () => Timer.clear(t), "double clear");

let t1, t2, t3, t4;
t1 = Timer.set(() => {
	Timer.clear(t2);
}, 2);

t2 = Timer.set(() => {
	$DONE("t2 should not be called");
}, 4);

t3 = Timer.set(() => {
	$DONE("t3 should not be called");
}, 3);
Timer.clear(t3);

// crash prior to commit c317ea0
t4 = Timer.set(() => {
	Timer.set(() => {
		assert.throws(SyntaxError, () => Timer.clear(t4), "clear one-shot after auto-clear");
	}, 1);
}, 1);


Timer.set(() => $DONE(), 8);
