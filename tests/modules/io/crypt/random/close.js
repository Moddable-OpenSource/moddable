/*---
description:
flags: [module]
---*/

import Random from "./random_FIXTURE.js";

const r = new Random({});
r.close();
r.close();		// double close is a no-op
assert.throws(Error, () => r.read(16), "read after close");
assert.throws(Error, () => r.read(new Uint8Array(16)), "read(buffer) after close");
assert.sameValue(r.format, "buffer", "format readable after close");
