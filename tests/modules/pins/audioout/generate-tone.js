/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

let mixer;

mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});

const popA = new SharedArrayBuffer(2);
(new Int16Array(popA))[0] = 32767;

const initial = -11_111;
mixer.enqueue(0, Mixer.Tone, 300, 512, initial);

let result = mixer.mix(600);

let samples = new Int16Array(result);
for (let i = 0; i < 512; i++) {
	const expected = (Math.idiv(i, 20) & 1) ? -initial : initial;
	assert.sameValue(samples[i], expected);
}
for (let i = 512; i < 600; i++)
	assert.sameValue(samples[i], 0);

mixer.enqueue(0, Mixer.RawSamples, popA, 1, 0, 1);
mixer.enqueue(0, Mixer.Tone, 600, 256, initial);
mixer.enqueue(0, Mixer.RawSamples, popA, 1, 0, 1);

result = mixer.mix(259);

samples = new Int16Array(result);
assert.sameValue(samples[0], 32767);		// popA
for (let i = 1; i < 257; i++) {
	const expected = (Math.idiv(i - 1, 10) & 1) ? -initial : initial;
	assert.sameValue(samples[i], expected);
}
assert.sameValue(samples[257], 32767);		// popA
assert.sameValue(samples[258], 0);		// silence 
