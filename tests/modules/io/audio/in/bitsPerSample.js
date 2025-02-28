/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});
let bitsPerSample = out.bitsPerSample;
assert.sameValue(typeof bitsPerSample, "number");
assert((8 === bitsPerSample) || (bitsPerSample === 16), "bitsPerSample invalid");
assert.throws(TypeError, () => out.bitsPerSample = bitsPerSample, "bitsPerSample is read-only");
out.close();
