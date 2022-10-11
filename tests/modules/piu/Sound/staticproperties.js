/*---
description: 
flags: [module]
---*/

import Sound from "piu/Sound"

Sound.volume = 0.5;
assert.sameValue(Sound.volume, 0.5, "volume");

Sound.volume = 0;
assert.sameValue(Sound.volume, 0, "volume");

Sound.volume = 2;
assert.sameValue(Sound.volume, 2, "volume");

Sound.volume = 1;
assert.sameValue(Sound.volume, 1, "volume");

assert.sameValue(Sound.bitsPerSample, 16, "bitsPerSample");
assert.sameValue(Sound.numChannels, 1, "numChannels");
assert.sameValue(Sound.sampleRate, 12000, "sampleRate");

assert.throws(TypeError, () => Sound.bitsPerSample = 0, "can't set bitsPerSample");
assert.throws(TypeError, () => Sound.numChannels = 0, "can't set numChannels");
assert.throws(TypeError, () => Sound.sampleRate = 0, "can't set sampleRate");
