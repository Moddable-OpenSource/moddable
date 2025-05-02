/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({});
assert.sameValue(typeof input.audioType, "string");
assert.sameValue(input.audioType, "LPCM");
input.close();

input = new AudioIn({audioType: "LPCM"});
assert.sameValue(typeof input.audioType, "string");
assert.sameValue(input.audioType, "LPCM");

assert.throws(TypeError, () => input.audioType = "xyzzy");

input.close();
