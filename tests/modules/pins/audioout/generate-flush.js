/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

const mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});

const initial = 4096;
mixer.enqueue(0, Mixer.Tone, 330, 100, initial);
mixer.enqueue(0, Mixer.Flush);
mixer.enqueue(0, Mixer.Tone, 220, 50, initial / 2);
verifyOutput(50, initial / 2);
verifyOutput(50, 0);

mixer.enqueue(0, Mixer.Tone, 330, 100, initial);
mixer.enqueue(0, Mixer.Volume, 128);
mixer.enqueue(0, Mixer.Tone, 220, 50, initial / 2);
mixer.enqueue(0, Mixer.Flush);
mixer.enqueue(0, Mixer.Tone, 330, 100, initial * 2);
verifyOutput(100, initial * 2);

mixer.enqueue(0, Mixer.Tone, 220, 50, initial / 2);
mixer.enqueue(0, Mixer.Flush);
verifyOutput(100, 0);

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
