/*---
description: 
flags: [module,async]
---*/

import Sound from "piu/Sound"

const samples = Sound.sampleRate / 6;

assert.throws(RangeError, () => {
	(new Sound([
		{frequency: 587.33, samples}
	])).play(12);
}, "invalid stream");

assert.throws(Error, () => {
	(new Sound([
		{frequency: 1, samples}
	])).play(0);
}, "invalid frequency");

assert.throws(Error, () => {
	(new Sound([
		{frequency: 587.33, samples}
	])).play(0, 0);
}, "invalid repeat");

assert.throws(Error, () => {
	(new Sound([
		{frequency: 587.33, samples}
	])).play(0, 12);
}, "invalid repeat");

Sound.volume = 0.25;
let tones = new Sound([
	{frequency: 466.16, samples},
	{frequency: 587.33, samples},
	{frequency: 698.46, samples}
]);

tones.play(0, 1, function() {
	Sound.volume = 1;
	tones = new Sound([
		{frequency: 587.33, samples},
		{frequency: 698.46, samples},
		{frequency: 932.33, samples: samples * 2}
	]);
	tones.play(0, 1, function() {
		$DONE();
	});
});
