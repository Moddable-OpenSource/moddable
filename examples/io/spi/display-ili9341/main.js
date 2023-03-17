/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

/*
	Warning: This is test code! Work in progress.
*/

import Display from "embedded:display/LCD/ILI9341"
import Poco from "commodetto/Poco";
import Timer from "timer"

const d = new Display({
	display: {
		...device.SPI.default,
		select: device.pin.displaySelect,
		active: 0
	},
	dc: {
		io: device.io.Digital,
		pin: device.pin.displayDC
	}
});

d.configure({
	format: 7,		// 16-bit RGB 5:6:5 little-endian
	async: true
});

//@@ patch in two properties for Poco compatibility
d.pixelsToBytes = function(pixels) {
	return pixels << 1;
}
d.pixelFormat = 7;
d.async = true;

if (0) {
	//@@ patch in functions required by Piu
	d.start = function (interval) {
		let timer = this.timer;
		if (!timer) {
			timer = this.timer = Timer.set(() => {
				this.context.onIdle();
			}, 1, 100);
			Timer.schedule(timer);
		}

		if (interval <= 5)
			interval = 5;
		if (timer.interval === interval)
			return;

		Timer.schedule(timer, interval, interval);
		timer.interval = interval;
	}
	d.stop = function () {
		const timer = this.timer;
		if (!timer) return;

		Timer.schedule(timer);
		delete timer.interval;
	}
	globalThis.screen = d;

	globalThis.application = new Application(null, {
		displayListLength: 2048, commandListLength: 2048,
		skin: new Skin({ fill: "white" })
	});

	let animatedContent = new Content(null, {
		height: 100, width: 100, loop: true,
		skin: new Skin({ fill: ["red", "yellow", "blue"] }),
		Behavior: class extends Behavior {
			onCreate(content) {
				this.startAnimation(content);
			}
			startAnimation(content) {
				content.duration = 3000;
				content.time = 0;
				content.start();
			}
			onTimeChanged(content) {
				content.state = content.fraction*2;
			}
		}
	});
	application.add(animatedContent);
}
else {
	const render = new Poco(d);

	let r = render.rectangle(10, 20, 50, 60);
	render.adaptInvalid(r);

	render.begin();
		render.fillRectangle(0xF800, 0, 0, render.width, render.height);
		render.fillRectangle(0x001F, 30, 30, render.width - 60, render.height - 60);
		render.fillRectangle(0x07E0, 60, 60, render.width - 120, render.height - 120);
	render.end();

	d.configure({async: false});

	const width = d.width, height = d.height;

	fill(0xFFFF, 0, 0, width, height, false, false);

	fill(0xF800, 0, 0, width, height, true, true);
	d.configure({
		invert: true
	});

	fill(0x001F, 0, 0, width, height);
	d.configure({
		invert: false
	});
	fill(0x07E0, 0, 0, width, height);

	fill(0xF81F, 0, 0, width / 2, height / 2);
	fill(0x07FF, width / 2, height / 2, width / 2, height / 2);

	fill(0xFFFF, 0, 0, 20, 20);

	let start = Date.now();
	stripes(0, 0, width, height);
	trace("sync stripes: ", Date.now() - start, "\n");

	d.configure({async: true});
	start = Date.now();
	stripes(0, 0, width, height);
	trace("async stripes: ", Date.now() - start, "\n");

	d.close();

	function fill(color, x, y, width, height, doEnd = true, doContinue = false) {
		if (doContinue)
			d.begin({x, y, width, height, continue: doContinue});
		else
			d.begin({x, y, width, height});

			const lines = 4;
			const pixels = new Uint16Array(width * lines);
			pixels.fill(color);

			for (; height > 0; height -= lines)
				d.send(pixels.buffer);
		if (doEnd)
			d.end();
	}
}

function stripes(x, y, width, height) {
	let color = (Math.random() * 65535) | 0;

	d.begin({x, y, width, height});

	const lines = 2;
	let pixelsA = new Uint16Array(new SharedArrayBuffer(width * lines * 2));
	let pixelsB = new Uint16Array(new SharedArrayBuffer(width * lines * 2));

	for (; height > 0; height -= lines) {
		pixelsA.fill(color);
		d.send(pixelsA.buffer);

		const t = pixelsA;
		pixelsA = pixelsB;
		pixelsB = t;
		color += 0x08041
	}
	d.end();
}
