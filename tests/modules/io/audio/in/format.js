/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audioin";

let input = new AudioIn({});
assert.throws(RangeError, () => input.format = "123", "invalid format - 123");
input.close();

input = new AudioIn({format: "buffer"});
assert.sameValue(input.format, "buffer");
input.close();

assert.throws(RangeError, () => new AudioIn({format: "number"}), "invalid format - number");
assert.throws(RangeError, () => new AudioIn({format: "xyzzy"}), "invalid format - xyzzy");
