/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

let mixer;

mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});

const popA = new SharedArrayBuffer(2);
(new Int16Array(popA))[0] = 32767;
const popB = new SharedArrayBuffer(2);
(new Int16Array(popB))[0] = -32768;
popA.delay = 129;
popB.delay = 17;

mixer.enqueue(0, Mixer.Silence, popA.delay);
mixer.enqueue(0, Mixer.RawSamples, popA, 1, 0, 1);
mixer.enqueue(0, Mixer.Silence, popB.delay);
mixer.enqueue(0, Mixer.RawSamples, popB, 1, 0, 1);
assert.sameValue(mixer.length(0), 0);
let result = mixer.mix(popA.delay + 1 + popB.delay + 1);
assert.sameValue(mixer.length(0), 4);
assert.sameValue(result.byteLength, 2 * (popA.delay + 1 + popB.delay + 1));

let samples = new Int16Array(result);
for (let i = 0; i < samples.length; i++) {
	let expect = 0;
	if (popA.delay === i)
		expect = 32767;
	else if ((popA.delay + 1 + popB.delay) === i)
		expect = -32768;
	assert.sameValue(samples[i], expect);
//	trace(`${i}: ${samples[i]}\n`);
}
