/*---
description: 
flags: [module,async]
---*/

import AudioOut from "pins/audioout"

const chorus = 1.005;

const tones = [
	[
		[466.16, 1],
		[587.33, 1],
		[698.46, 1],
		[587.33, 1],
		[698.46, 1],
		[932.33, 2],
		[0, 1],
		[233.08, 4],

		[466.16 * chorus, 1],
		[493.88 * chorus, 1],
		[466.16 * chorus, 1],
		[493.88 * chorus, 1],
		[466.16 * chorus, 4],
	],
	[
		[932.33, 2],
		[698.46, 1],
		[587.33, 1],
		[698.46, 1],
		[587.33, 1],
		[466.16, 1],
		[0, 2],
		[174.61, 1],
		[146.83, 1],
		[116.54, 1],

		[466.16, 1],
		[493.88, 1],
		[466.16, 1],
		[493.88, 1],
		[466.16, 4],
	]
];

const beat = 1200;

const out = new AudioOut({streams: 2, sampleRate: 12000, numChannels: 1});
out.callbacks = [
	function(value) {
		if (value)
			next(0);
		else if (0 === --out.remain)
			$DONE();
	},
	function(value) {
		if (value)
			next(1);
		else if (0 === --out.remain)
			$DONE();
	}
];
out.remain = tones.length;

next(0);
next(1);
next(0);
next(1);
out.start();

function next(stream) {
	if (!tones[stream].length)
		return;

	const tone = tones[stream].shift();
	if (tone[0])
		out.enqueue(stream, AudioOut.Tone, tone[0], tone[1] * beat);
	else
		out.enqueue(stream, AudioOut.Silence, tone[1] * beat);
	out.enqueue(stream, AudioOut.Callback, tones[stream].length);
}
