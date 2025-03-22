/*---
description: 
flags: [module]
---*/

import OTA from "ota";
import Flash from "flash";

let o = new OTA;
let f = new Flash("running");

o.write(f.read(0, f.blockSize));

o.cancel();

assert.throws(Error, () => o.write(f.read(f.blockSize, f.blockSize)));

assert.throws(Error, () => o.complete());

o.cancel();		// safe to call more than once
