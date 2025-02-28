/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

[8000, 22050, 24000, 32000, 44100, 48000].forEach(sampleRate => {
	let out = new AudioOut({
		sampleRate
	});
	assert.sameValue(out.sampleRate, sampleRate, "sample rate option " + sampleRate);
	out.close();
});

[1, 2].forEach(channels => {
	let out = new AudioOut({
		channels
	});
	assert.sameValue(out.channels, channels, "channels option " + channels);
	out.close();
});


[8, 16].forEach(bitsPerSample => {
	let out = new AudioOut({
		bitsPerSample
	});
	assert.sameValue(out.bitsPerSample, bitsPerSample, "bitsPerSample option " + bitsPerSample);
	out.close();
});

let out = new AudioOut({
	bitsPerSample: "16",
	channels: "2",
	sampleRate: "44100",
	audioType: "LPCM"
});
assert.sameValue(out.bitsPerSample, 16, "bitsPerSample option 16");
assert.sameValue(out.channels, 2, "channels option 2");
assert.sameValue(out.sampleRate, 44100, "sampleRate option 44100");

assert.throws(RangeError, () => new AudioOut({bitsPerSample: -1}), "bitsPerSample -1");
assert.throws(RangeError, () => new AudioOut({bitsPerSample: 0}), "bitsPerSample 0");
assert.throws(RangeError, () => new AudioOut({bitsPerSample: 17}), "bitsPerSample 17");
assert.throws(RangeError, () => new AudioOut({bitsPerSample: 65536 + 8}), "bitsPerSample 65536 + 8");
assert.throws(TypeError, () => new AudioOut({bitsPerSample: Symbol()}), "bitsPerSample symbol");

assert.throws(RangeError, () => new AudioOut({channels: -1}), "channels -1");
assert.throws(RangeError, () => new AudioOut({channels: 0}), "channels 0");
assert.throws(RangeError, () => new AudioOut({channels: 3}), "channels 3");
assert.throws(RangeError, () => new AudioOut({channels: 65537}), "channels 65537");
assert.throws(TypeError, () => new AudioOut({channels: Symbol()}), "channels symbol");

assert.throws(RangeError, () => new AudioOut({sampleRate: -1}), "sampleRate -1");
assert.throws(RangeError, () => new AudioOut({sampleRate: 0}), "sampleRate 0");
assert.throws(RangeError, () => new AudioOut({sampleRate: 100_000}), "sampleRate 100_000");
assert.throws(TypeError, () => new AudioOut({sampleRate: Symbol()}), "sampleRate symbol");

assert.throws(RangeError, () => new AudioOut({audioType: "xyzzy"}), "audioType xyzzy");
