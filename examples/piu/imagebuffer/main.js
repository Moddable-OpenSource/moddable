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
import BufferOut from "commodetto/BufferOut";
import Poco from "commodetto/Poco";

const SIZE = 40;
const STEP = 4;
const COLUMNS = 4;
const ROWS = 4;
const GAP = 8;

class GridBehavior extends Behavior {
	onCreate(container) {
		// Commodetto renders offscreen into one small buffer
		const offscreen = new BufferOut({ width: SIZE, height: SIZE, pixelFormat: screen.pixelFormat });
		const poco = new Poco(offscreen);
		this.offscreen = offscreen;
		this.poco = poco;
		this.colors = [
			poco.makeColor(255, 0, 0),
			poco.makeColor(255, 128, 0),
			poco.makeColor(255, 255, 0),
			poco.makeColor(0, 192, 0),
			poco.makeColor(0, 128, 255),
			poco.makeColor(128, 0, 255),
		];
		this.phase = 0;
	}
	onDisplaying(container) {
		// a grid of image buffers that all display the same buffer
		const left = (container.width - (COLUMNS * SIZE) - ((COLUMNS - 1) * GAP)) >> 1;
		const top = (container.height - (ROWS * SIZE) - ((ROWS - 1) * GAP)) >> 1;
		for (let row = 0; row < ROWS; row++) {
			for (let column = 0; column < COLUMNS; column++) {
				container.add(new ImageBuffer(null, {
					left: left + (column * (SIZE + GAP)), top: top + (row * (SIZE + GAP)),
					imageWidth: SIZE, imageHeight: SIZE
				}));
			}
		}
		this.draw(container);
		container.interval = 100;
		container.start();
	}
	onTimeChanged(container) {
		this.phase += 1;
		this.draw(container);
	}
	draw(container) {
		const poco = this.poco;
		const colors = this.colors;
		poco.begin();
			for (let inset = 0, index = this.phase; inset < (SIZE >> 1); inset += STEP, index += 1) {
				const size = SIZE - (inset << 1);
				poco.fillRectangle(colors[index % colors.length], inset, inset, size, size);
			}
		poco.end();
		// setting the buffer, even to the same buffer, updates the screen
		const buffer = this.offscreen.buffer;
		for (let image = container.first; image; image = image.next)
			image.buffer = buffer;
	}
}

new Application(null, {
	skin: new Skin({
		fill: "black"
	})
});

application.add(new Container(null, {
	left: 0, right: 0, top: 0, bottom: 0,
	Behavior: GridBehavior
}));
