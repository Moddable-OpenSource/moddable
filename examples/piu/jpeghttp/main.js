/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";
import {} from "piu/ImageBuffer";
import loadJPEG from "commodetto/loadJPEG";
import { fetch } from "fetch";

const urls = [
	"http://test.moddable.com/example/images/example1.jpg",
	"http://test.moddable.com/example/images/example2.jpg",
	"http://test.moddable.com/example/images/example3.jpg",
	"http://test.moddable.com/example/images/example4.jpg",
];

class JPEGBehavior extends Behavior {
	onCreate(container) {
		this.index = 0;
	}
	onDisplaying(container) {
		this.load(container);
	}
	onFinished(container) {
		this.load(container);
	}
	async load(container) {
		const spinner = container.next;
		const url = urls[this.index];
		this.index = (this.index + 1) % urls.length;
		spinner.visible = true;
		spinner.start();
		try {
			const response = await fetch(url);
			const data = await response.arrayBuffer();
			container.empty();
			const bitmap = loadJPEG(data);
			const image = new ImageBuffer(null, { imageWidth: bitmap.width, imageHeight: bitmap.height });
			image.buffer = bitmap.buffer;
			container.add(image);
		}
		catch (e) {
			trace(`${url}: ${e}\n`);
		}
		spinner.stop();
		spinner.visible = false;
		// show the next JPEG image in a few seconds
		container.duration = 3000;
		container.time = 0;
		container.start();
	}
}

class SpinnerBehavior extends Behavior {
	onCreate(spinner) {
		this.step = 0;
		spinner.interval = 150;
	}
	onTimeChanged(spinner) {
		const step = this.step = (this.step + 1) & 3;
		spinner.first.coordinates = {
			left: ((1 === step) || (2 === step)) ? 10 : 0,
			top: (step >= 2) ? 10 : 0,
			width: 10, height: 10
		};
	}
}

new Application(null, {
	skin: new Skin({
		fill: "black"
	})
});

application.add(new Column(null, {
	contents: [
		new Container(null, { Behavior: JPEGBehavior }),
		new Container(null, {
			top: 10, width: 20, height: 20, visible: false,
			Behavior: SpinnerBehavior,
			contents: [
				new Content(null, { left: 0, top: 0, width: 10, height: 10, skin: new Skin({ fill: "white" }) }),
			]
		}),
	]
}));
