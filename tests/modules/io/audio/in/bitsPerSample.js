/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({});
let bitsPerSample = input.bitsPerSample;
assert.sameValue(typeof bitsPerSample, "number");
assert((8 === bitsPerSample) || (bitsPerSample === 16), "bitsPerSample invalid");
assert.throws(TypeError, () => input.bitsPerSample = bitsPerSample, "bitsPerSample is read-only");
input.close();
