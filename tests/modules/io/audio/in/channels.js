/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({});
let channels = input.channels;
assert.sameValue(typeof channels, "number");
assert((1 <= channels) && (channels <= 2), "channels input of range");
assert.throws(TypeError, () => input.channels = channels, "channels is read-only");
input.close();
