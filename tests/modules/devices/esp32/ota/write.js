/*---
description: 
flags: [module]
---*/

import OTA from "ota";
import Flash from "flash";

const o = new OTA;
const f = new Flash("running");


o.write(f.read(0, f.blockSize));
o.write(new Uint8Array(f.read(0, f.blockSize)));
assert.throws(SyntaxError, () => o.write());
assert.throws(TypeError, () => o.write(1));
assert.throws(TypeError, () => o.write("not a number"));
assert.throws(TypeError, () => o.write({}));
assert.throws(TypeError, () => o.write(new Uint32Array(1)));

o.cancel();
