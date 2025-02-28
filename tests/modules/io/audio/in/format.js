/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});
assert.sameValue(typeof out.volume, "number");
assert.throws(RangeError, () => out.format = "123", "invalid format - 123");
out.close();

out = new AudioOut({format: "buffer"});
assert.sameValue(out.format, "buffer");
out.close();

assert.throws(RangeError, () => new AudioOut({format: "number"}), "invalid format - number");
assert.throws(RangeError, () => new AudioOut({format: "xyzzy"}), "invalid format - xyzzy");
