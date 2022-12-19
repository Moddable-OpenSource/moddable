/*---
description: 
flags: [module]
---*/

import Time from "time";

let d = new Date;

Time.timezone = -8 * 60 * 60;

Time.dst = -60 * 60;
assert.sameValue(Time.dst, -60 * 60);
assert(d.toString().endsWith(" GMT-0900"));

Time.dst = 2 * 60 * 60;
assert.sameValue(Time.dst, 2 * 60 * 60);
assert(d.toString().endsWith(" GMT-0600"));

Time.dst = 0;
assert.sameValue(Time.dst, 0);
assert(d.toString().endsWith(" GMT-0800"));

Time.dst = "3600";
assert.sameValue(Time.dst, 3600);
assert(d.toString().endsWith(" GMT-0700"));

assert.throws(TypeError, () => Time.dst = 60n, "reject bigint");
