/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

const mixer = new Mixer({streams: 2, sampleRate: 12000, numChannels: 1});

let initial = +11_111;

mixer.enqueue(0, Mixer.Tone, 300, 800, initial);
mixer.enqueue(1, Mixer.Tone, 300, 400, -initial);
mixer.enqueue(1, Mixer.Silence, 400);

let result = mixer.mix(400);
let samples = new Int16Array(result);
for (let i = 0; i < 400; i++)
	assert.sameValue(samples[i], 0);

result = mixer.mix(400);
samples = new Int16Array(result);
for (let i = 400; i < 800; i++) {
	const expected = (Math.idiv(i, 20) & 1) ? -initial : initial;
	assert.sameValue(samples[i - 400], expected);
}

// clamping on overdrive mix
mixer.enqueue(0, Mixer.Flush);
initial = 16000;
mixer.enqueue(0, Mixer.Volume, 512);
mixer.enqueue(0, Mixer.Tone, 300, 400, initial);
mixer.enqueue(1, Mixer.Tone, 300, 399, initial);

result = mixer.mix(400);
samples = new Int16Array(result);
for (let i = 0; i < 399; i++) {
	const expected = (Math.idiv(i, 20) & 1) ? -32768 : 32767;
	assert.sameValue(samples[i], expected);
}
const expected = (Math.idiv(399, 20) & 1) ? -32000 : 32000;
assert.sameValue(samples[399], expected);
