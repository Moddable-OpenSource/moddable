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

import Display from "embedded:display/LCD/ILI9341"
import Poco from "commodetto/Poco";
import Timer from "timer"

const d = new Display({
	display: {
		...device.SPI.default,
		select: device.pin.displaySelect
	},
	dc: {
		io: device.io.Digital,
		pin: device.pin.displayDC
	}
});

d.configure({
	format: 7		// 16-bit RGB 5:6:5 little-endian
})

//@@ patch in a two properties for Poco compatibility
d.pixelsToBytes = function(pixels) {
	return pixels << 1;
}
d.pixelFormat = 7;

const render = new Poco(d);

let r = render.rectangle(10, 20, 50, 60);
render.adaptInvalid(r);

render.begin();
	render.fillRectangle(0xF800, 0, 0, render.width, render.height);
	render.fillRectangle(0x001F, 30, 30, render.width - 60, render.height - 60);
	render.fillRectangle(0x07E0, 60, 60, render.width - 120, render.height - 120);
render.end();

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
fill(0x07FF, width / 2, height / 2, width, height);

fill(0xFFFF, 0, 0, 20, 20);

function fill(color, x, y, width, height, doEnd = true, doContinue = false) {
	if (doContinue)
		d.begin({x, y, width, height, continue: doContinue});
	else
		d.begin({x, y, width, height});

		const lines = 4;
		const pixels = new Uint16Array(width * lines);
		pixels.fill(color);

		for (; y < height; y += lines)
			d.send(pixels.buffer);
	if (doEnd)
		d.end();
}
