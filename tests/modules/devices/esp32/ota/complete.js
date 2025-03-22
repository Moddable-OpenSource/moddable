/*---
description: 
flags: [module]
---*/

import OTA from "ota";
import Flash from "flash";

const fr = new Flash("running");
const fn = new Flash("nextota");
const byteLength = Math.min(fr.byteLength, fn.byteLength, fn.byteLength); 		// partitions may not be identical size

assert.throws(Error, () => (new OTA).complete());

const o = new OTA;

trace("begin long OTA test\n")
for (let offset = 0, blockSize = fr.blockSize; offset < byteLength; offset += blockSize)
	o.write(fr.read(offset, blockSize));
trace("complete long OTA test\n")

o.complete();

assert.throws(Error, () => o.write(fr.read(0, fr.blockSize)));

