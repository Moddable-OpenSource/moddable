/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

const mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});

const initial = 4096;
mixer.enqueue(0, Mixer.Volume, 256);
mixer.enqueue(0, Mixer.Tone, 330, 100, initial);
verifyOutput(100, initial);

mixer.enqueue(0, Mixer.Volume, 128);
mixer.enqueue(0, Mixer.Tone, 330, 100, initial);
verifyOutput(100, initial / 2);

mixer.enqueue(0, Mixer.Volume, 64);
mixer.enqueue(0, Mixer.Tone, 252, 100, initial);
verifyOutput(100, initial / 4);

mixer.enqueue(0, Mixer.Volume, 512);
mixer.enqueue(0, Mixer.Tone, 523, 100, initial);
verifyOutput(100, initial * 2);

mixer.enqueue(0, Mixer.Volume, 0);
mixer.enqueue(0, Mixer.Tone, 440, 50, initial);
verifyOutput(100, 0);

mixer.enqueue(0, Mixer.Volume, 128);
mixer.enqueue(0, Mixer.Tone, 440, 157, initial);
mixer.enqueue(0, Mixer.Volume, 256);
mixer.enqueue(0, Mixer.Tone, 220, 162, initial);
verifyOutput(157, initial / 2);
verifyOutput(162, initial);

mixer.enqueue(0, Mixer.Tone, 440, 127, initial);
mixer.enqueue(0, Mixer.Volume, 0);
mixer.enqueue(0, Mixer.Volume, 128);
mixer.enqueue(0, Mixer.Tone, 440, 127, initial);
verifyOutput(127, initial);
verifyOutput(127, initial / 2);

function verifyOutput(count, expected) {
	const result = mixer.mix(count);
	const samples = new Int16Array(result);
	for (let i = 0; i < count; i++) {
		let value = samples[i];
		if (value < 0)
			value = -value;
		assert.sameValue(value, expected);
	}
	$262.gc();
}
