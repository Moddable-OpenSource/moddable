/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
assert.sameValue(f.format, "buffer");
f.close();


f = flash.open({path, format: "buffer"});
assert.sameValue(f.format, "buffer");

f.format = "buffer";
assert.throws(RangeError, () => f.format = "xyzzy");
assert.sameValue(f.format, "buffer", "should remain buffer");

f.close();
