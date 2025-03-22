/*
 * Copyright (c) 2016-2024 Moddable Tech, Inc.
 *
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

import {} from "piu/MC";
import Modules from "modules";

const sampleRate = 48_000;
const tones = [196.00, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77];
let AudioOut, audio;

if (Modules.has("pins/audioout")) {
	AudioOut = Modules.importNow("pins/audioout");
	audio = new AudioOut({streams: 2, sampleRate: sampleRate, numChannels: 1});
	audio.start();
}

const backgroundSkin = new Skin({
	texture: new Texture("island.png"),
	width: 240, height: 320,
});

const dimSkin = new Skin({
	texture: new Texture("led-dim.png"),
	color: "white",
	width: 40, height: 40,
});

const brightSkin = new Skin({
	texture: new Texture("led-bright.png"),
	color: "white",
	width: 48, height: 48,
});

const outlineSkin = new Skin({
	fill: "transparent", stroke: "white",
	borders: {top: 3, bottom: 3, left: 3, right: 3}
});

const transparentWhiteSkin = new Skin({
	fill: "#FFFFFFCC"
});

class DimmingBehavior extends Behavior {
	onDisplaying(content) {
		if (undefined === globalThis.backlight)
			throw new Error("backlight missing")
		this.adjustBrightness(content);
	}
	onTouchBegan(content, id, x, y) {
		if (audio)
			delete audio.last;
		this.onTouchMoved(content, id, x, y);
	}
	onTouchMoved(content, id, x, y) {
		const bounds = content.bounds;
		y = Math.max(bounds.y, y);
		y = Math.min(y, bounds.y + bounds.height);
		content.first.height = (bounds.y + bounds.height) - y;
		this.adjustBrightness(content);
	}
	onTouchEnded(content, id, x, y) {
		audio?.enqueue(0, AudioOut.Flush);
		audio?.enqueue(1, AudioOut.Flush);
	}
	adjustBrightness(content) {
		const floor = 0.075;
		let fraction = floor + ((1 - floor) * (content.first.height / content.height));
		backlight.write(fraction * 100);

		if (undefined === audio)
			;
		else if (undefined == audio.last)
			audio.last = -1;
		else {
			const index = ((tones.length - 6) * fraction) | 0;
			if (index != audio.last) {
				audio.enqueue(0, AudioOut.Flush);
				audio.enqueue(1, AudioOut.Flush);
				audio.enqueue(0, AudioOut.Tone, tones[index], sampleRate / 5);
				audio.enqueue(1, AudioOut.Tone, tones[index + 5], sampleRate / 10);
				audio.last = index;
			}
		}
	}
};

const ScreenDimmingApplication = Application.template($ => ({
	skin: backgroundSkin,
	contents: [
		Content($, {
			skin: brightSkin, top: 5
		}),
		Container($, {
			width: 100, height: 200, skin: outlineSkin,
			active: true, Behavior: DimmingBehavior,
			contents: [
				Content($, {
					left: 0, right: 0, bottom: 0, height: 150,
					skin: transparentWhiteSkin
				}),
			],
		}),
		Content($, {
			skin: dimSkin, bottom: 10
		}),
	]
}));

export default new ScreenDimmingApplication(null, { displayListLength:4096, touchCount:1 });
