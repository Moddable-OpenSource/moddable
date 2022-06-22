/*---
description: 
flags: [module]
---*/

import Time from "time";

assert.throws(SyntaxError, () => Time.set(), "Time.set requires one parameter");
assert.throws(TypeError, () => Time.set(1n), "Time.set rejects bigint");

let d = new Date("July 15, 2021");
Time.set(d / 1000);
assert.sameValue(Math.floor(Date.now() / 1000), Math.floor(d / 1000));

Time.set(String(d / 1000));
assert.sameValue(Math.floor(Date.now() / 1000), Math.floor(d / 1000));

d = new Date("July 15, 2031");
Time.set(d / 1000);
assert.sameValue((new Date).getUTCFullYear(), 2031);

d = new Date("July 15, 1971");
Time.set(d / 1000);
assert.sameValue((new Date).getUTCFullYear(), 1971);
