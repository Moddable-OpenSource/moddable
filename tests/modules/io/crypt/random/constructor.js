/*---
description:
flags: [module]
---*/

import Random from "./random_FIXTURE.js";

const r = new Random({});
assert.sameValue(r.format, "buffer", "format");
r.format = "buffer";
assert.throws(RangeError, () => { r.format = "number"; }, "unsupported format");
assert.throws(RangeError, () => { r.format = "string"; }, "unsupported format");
r.close();

assert.throws(TypeError, () => new Random(), "options required");
assert.throws(TypeError, () => new Random(undefined), "options undefined");
assert.throws(TypeError, () => new Random(null), "options null");
assert.throws(TypeError, () => new Random("buffer"), "options string");
new Random({io: Random}).close();		// the reserved io property is ignored

assert.sameValue(Random.prototype[Symbol.dispose], Random.prototype.close, "Symbol.dispose is close");
