/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
const status = f.status();

assert("blockLength" in status);
assert("blocks" in status);
assert("size" in status);

assert.sameValue(typeof status.blockLength, "number");
assert.sameValue(typeof status.blocks, "number");
assert.sameValue(typeof status.size, "number");

assert(status.blocks > 0);
assert(status.blockLength > 0);
assert.sameValue(status.blockLength * status.blocks, status.size);
