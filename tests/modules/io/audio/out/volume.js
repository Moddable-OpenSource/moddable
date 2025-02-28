/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audio/out";

let out = new AudioOut({});
let volume = out.volume;
assert.sameValue(typeof volume, "number");
assert((0.0 <= volume) && (volume <= 1.0), "volume out of range");

[0, 0.4, 0.5, 0.6, 1].forEach(volume => {
	out.volume = volume;
	assert.sameValue(Math.round(100 * out.volume), 100 * volume);
});

assert.throws(RangeError, () => out.volume = 1.1, "volume 1.1");
assert.throws(RangeError, () => out.volume = -1, "volume -1");

assert.throws(TypeError, () => out.volume = Symbol(), "set volume to symbol");
assert.throws(TypeError, () => out.volume = 1n, "set volume to bigint");

out.close();
