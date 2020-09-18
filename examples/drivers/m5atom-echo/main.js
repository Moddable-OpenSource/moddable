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

let playing;

global.speaker = new AudioOut({
	streams: 1
});
speaker.callback = function () {
	this.stop();
	trace('Speaker Stopped!\n');
	playing = false;
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
	playing = true;
	trace('play:', count, '\n');
}

function play(index) {
	speaker.stop();
	speaker.enqueue(0, AudioOut.Flush);
	speaker.enqueue(0, AudioOut.Samples, new Resource(clips[index]));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();
}

const value = [0, 0, 0];
let step = 15;
let index = 0;
Timer.repeat(() => {
	if (playing)
		np.setPixel(0, np.makeRGB(255, 255, 255));
	else {
		value[index] += step;
		if (step < 0) {
			if (0 === value[index]) {
				index = (index + 1) % value.length;
				step = -step;
			}
		}
		else if (255 === value[index])
			step = -step;
		np.setPixel(0, np.makeRGB.apply(np, value));
	}
	np.update();
}, 17);
