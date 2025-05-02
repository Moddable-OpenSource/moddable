/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

[8000, 22050, 24000, 32000, 44100, 48000].forEach(sampleRate => {
	let input = new AudioIn({
		sampleRate
	});
	assert.sameValue(input.sampleRate, sampleRate, "sample rate option " + sampleRate);
	input.close();
});

[1, 2].forEach(channels => {
	let input = new AudioIn({
		channels
	});
	assert.sameValue(input.channels, channels, "channels option " + channels);
	input.close();
});


[8, 16].forEach(bitsPerSample => {
	let input = new AudioIn({
		bitsPerSample
	});
	assert.sameValue(input.bitsPerSample, bitsPerSample, "bitsPerSample option " + bitsPerSample);
	input.close();
});

let input = new AudioIn({
	bitsPerSample: "16",
	channels: "2",
	sampleRate: "44100",
	audioType: "LPCM"
});
assert.sameValue(input.bitsPerSample, 16, "bitsPerSample option 16");
assert.sameValue(input.channels, 2, "channels option 2");
assert.sameValue(input.sampleRate, 44100, "sampleRate option 44100");

assert.throws(RangeError, () => new AudioIn({bitsPerSample: -1}), "bitsPerSample -1");
assert.throws(RangeError, () => new AudioIn({bitsPerSample: 0}), "bitsPerSample 0");
assert.throws(RangeError, () => new AudioIn({bitsPerSample: 17}), "bitsPerSample 17");
assert.throws(RangeError, () => new AudioIn({bitsPerSample: 65536 + 8}), "bitsPerSample 65536 + 8");
assert.throws(TypeError, () => new AudioIn({bitsPerSample: Symbol()}), "bitsPerSample symbol");

assert.throws(RangeError, () => new AudioIn({channels: -1}), "channels -1");
assert.throws(RangeError, () => new AudioIn({channels: 0}), "channels 0");
assert.throws(RangeError, () => new AudioIn({channels: 3}), "channels 3");
assert.throws(RangeError, () => new AudioIn({channels: 65537}), "channels 65537");
assert.throws(TypeError, () => new AudioIn({channels: Symbol()}), "channels symbol");

assert.throws(RangeError, () => new AudioIn({sampleRate: -1}), "sampleRate -1");
assert.throws(RangeError, () => new AudioIn({sampleRate: 0}), "sampleRate 0");
assert.throws(RangeError, () => new AudioIn({sampleRate: 100_000}), "sampleRate 100_000");
assert.throws(TypeError, () => new AudioIn({sampleRate: Symbol()}), "sampleRate symbol");

assert.throws(RangeError, () => new AudioIn({audioType: "xyzzy"}), "audioType xyzzy");
