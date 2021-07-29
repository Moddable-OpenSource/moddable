/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 * Copyright (c) Wilberforce
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import Resource from "Resource";
import AudioOut from "pins/audioout";

let playing = false;
let count = 0;

const speaker = new AudioOut({streams: 1});

speaker.callback = function () {
	this.stop();
	trace('Speaker Stopped!\n');
	playing = false;
};

const clips = [
	'bflatmajor',
	'wilhelm-scream',
	'magic-sound'
];

button.a.onChanged = function () {
	if (button.a.read()) {
		return;
	}
	play(count);
	trace('Play: ', clips[count], '\n');
	count = (count + 1) % clips.length;
	playing = true;
}

function play(index) {
	speaker.stop();
	speaker.enqueue(0, AudioOut.Flush);
	speaker.enqueue(0, AudioOut.Samples, new Resource(clips[index] + ".maud"));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();
}

const value = [0, 0, 0];
let step = 15;
let index = 0;
Timer.repeat(() => {
	if (playing)
		lights.setPixel(0, lights.makeRGB(255, 255, 255));
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
		lights.setPixel(0, lights.makeRGB.apply(lights, value));
	}
	lights.update();
}, 17);
