/*---
description: 
flags: [module,async]
---*/

import Sound from "piu/Sound"

const path = "bflatmajor-ima-12000.wav";
const wav1 = new Sound({path});
const wav2 = new Sound({path, offset: 0, size: 6000});
const wav3 = new Sound({path, offset: 6000});

assert.throws(RangeError, () => wav1.play(12),  "invalid stream");
assert.throws(Error, () => wav1.play(0, -1),  "invalid repeat");

Sound.volume = 0.25;
wav1.play(0, 1, function() {
	Sound.volume = 0.66;
	wav2.play(0, 1, function() {
		Sound.volume = 1.0;
		wav3.play(0, 1, function() {
			$DONE();
		});
	});
});
