/*---
description: 
flags: [module]
---*/

import Time from "time";

let d = new Date;

Time.dst = 0;
Time.timezone = 0;
assert.sameValue(Time.timezone, 0);
assert.sameValue(d.getTimezoneOffset(), 0);
assert(d.toTimeString().endsWith(" GMT+0000"));

Time.timezone = String(60 * 60);
assert.sameValue(Time.timezone, 60 * 60);
assert.sameValue(d.getTimezoneOffset(), -60);
assert(d.toTimeString().endsWith(" GMT+0100"));

Time.timezone = -60 * 60;
assert.sameValue(Time.timezone, -60 * 60);
assert.sameValue(d.getTimezoneOffset(), 60);
assert(d.toTimeString().endsWith(" GMT-0100"));



Time.dst = 60 * 60;
Time.timezone = 0;
assert.sameValue(Time.timezone, 0);
assert.sameValue(d.getTimezoneOffset(), -60);
assert(d.toTimeString().endsWith(" GMT+0100"));

Time.timezone = String(60 * 60);
assert.sameValue(Time.timezone, 60 * 60);
assert.sameValue(d.getTimezoneOffset(), -120);
assert(d.toTimeString().endsWith(" GMT+0200"));

Time.timezone = -60 * 60;
assert.sameValue(Time.timezone, -60 * 60);
assert.sameValue(d.getTimezoneOffset(), 0);
assert(d.toTimeString().endsWith(" GMT+0000"));


assert.throws(TypeError, () => Time.timezone = 60n * 60n, "reject bigint");
