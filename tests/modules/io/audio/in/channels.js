/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});
let channels = out.channels;
assert.sameValue(typeof channels, "number");
assert((1 <= channels) && (channels <= 2), "channels out of range");
assert.throws(TypeError, () => out.channels = channels, "channels is read-only");
out.close();
