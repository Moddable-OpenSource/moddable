/*---
description: 
flags: [module]
---*/

import Time from "time";
import Timer from "timer";

assert.throws(TypeError, () => Time.ticks = 12, "Time.ticks is read-only");

let start = Time.ticks;
Timer.delay(10);
let delta = Time.ticks - start - 10;
assert((delta >= -1) && (delta <= 1), "ticks off");

start = Time.ticks;
Timer.delay(4);
delta = Time.ticks - start - 4;
assert((delta >= -1) && (delta <= 1), "ticks off");
