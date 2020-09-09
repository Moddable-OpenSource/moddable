/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 * Copyright (c) Wilberforce
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

let count = 0;

import NeoPixel from "neopixel";
import Timer from "timer";

import AudioOut from "pins/audioout";
import Resource from "Resource";

const np = new NeoPixel({
	length: 1,
	pin: 27,
	order: "RGB"
});

global.speaker = new AudioOut({
	streams: 2
});
speaker.callback = function () {
	this.stop();
	trace('Speaker Stopped!');
};

let clips = [
	'bflatmajor.maud',
	'wilhelm-scream.maud',
	'magic-sound.maud'
];

button.a.onChanged = function () {
	if (button.a.read()) {
		return;
	}
	play(count);
	count = (count + 1) % clips.length;
	trace('play:', count, '\n');
}

function play(index) {
	speaker.stop();
	speaker.enqueue(0, AudioOut.Samples, new Resource(clips[index]));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();
}

let value = 0x01;
Timer.repeat(() => {
	let v = value;
	for (let i = 0; i < np.length; i++) {
		v <<= 1;
		if (v == (1 << 24))
			v = 1;
		np.setPixel(i, v);
	}

	np.update();

	value <<= 1;
	if (value == (1 << 24))
		value = 1;
}, 400);