/*---
description: 
flags: [module,async]
---*/

import Sound from "piu/Sound"

assert.throws(RangeError, () => Sound.open(12), "invalid streams");
assert.throws(RangeError, () => Sound.open(-1), "invalid streams");
assert.throws(TypeError, () => Sound.open(new Symbol), "invalid streams");

const path = "bflatmajor-ima-12000.wav";
const wav = new Sound({path});
const samples = Sound.sampleRate / 6;
const tones = new Sound([
		{frequency: 587.33, samples},
		{frequency: 698.46, samples},
		{frequency: 932.33, samples: samples * 2}
	]);

let streams = 2;
Sound.open(streams);

wav.play(0, 1, function() {
	if (!--streams) {
		Sound.close();
		$DONE()
	}
});

tones.play(1, 1, function() {
	if (!--streams) {
		Sound.close();
		$DONE()
	}
});
