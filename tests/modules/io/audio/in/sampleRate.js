/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});
let sampleRate = out.sampleRate;
assert.sameValue(typeof sampleRate, "number");
assert((0 < sampleRate) && (sampleRate < 65536), "sampleRate out of range");
assert.throws(TypeError, () => out.sampleRate = sampleRate, "sampleRate is read-only");
out.close();
