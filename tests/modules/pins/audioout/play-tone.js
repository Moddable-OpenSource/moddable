/*---
description: 
flags: [module,async]
---*/

import AudioOut from "pins/audioout"

const tones = [
	[466.16, 1],
	[587.33, 1],
	[698.46, 1],
	[587.33, 1],
	[698.46, 1],
	[932.33, 2]
];

const beat = 1200;

const out = new AudioOut({streams: 1, sampleRate: 12000, numChannels: 1});
out.callback = function(value) {
	if (0 === value)
		$DONE();
	else
		next();
}

next();
next();
out.start();

function next() {
	if (!tones.length)
		return;

	const tone = tones.shift();
	out.enqueue(0, AudioOut.Tone, tone[0], tone[1] * beat);
	out.enqueue(0, AudioOut.Callback, tones.length);
}
