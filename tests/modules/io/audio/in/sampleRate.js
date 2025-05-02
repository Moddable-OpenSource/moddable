/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({});
let sampleRate = input.sampleRate;
assert.sameValue(typeof sampleRate, "number");
assert((0 < sampleRate) && (sampleRate < 65536), "sampleRate out of range");
assert.throws(TypeError, () => input.sampleRate = sampleRate, "sampleRate is read-only");
input.close();
