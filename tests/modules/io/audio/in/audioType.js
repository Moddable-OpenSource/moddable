/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});
assert.sameValue(typeof out.audioType, "string");
assert.sameValue(out.audioType, "LPCM");
out.close();

out = new AudioOut({audioType: "LPCM"});
assert.sameValue(typeof out.audioType, "string");
assert.sameValue(out.audioType, "LPCM");

assert.throws(TypeError, () => out.audioType = "xyzzy");

out.close();
