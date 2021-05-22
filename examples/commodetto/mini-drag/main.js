/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
	N.B.
		Rectangle class modeled on FskRectangle.c (Apache License, Marvell Semiconductor)
*/

import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";

let render = new Poco(screen);

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);

let font = parseBMF(new Resource("OpenSans-Regular-20.bf4"));
let background = parseBMP(new Resource("desktop-color.bmp"));
let button = parseBMP(new Resource("button-color.bmp"));
button.alpha = parseBMP(new Resource("button-alpha.bmp"));

function drawBackground() {
	render.fillPattern(background, 0, 0, render.width, render.height, 0, 0, background.width, background.height);
}

class Rectangle {
	constructor(x, y, w, h) {
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
	}
	contains(x, y) {
		if (x >= this.x && x < this.x + this.w && y >= this.y && y < this.y + this.h)
			return true;
		return false;
	}
	static union(r1, r2) {
		let r = new Rectangle();
		if (r1.x > r2.x)
			r.x = r2.x;
		else
			r.x = r1.x;

		if (r1.y > r2.y)
			r.y = r2.y;
		else
			r.y = r1.y;

		let v1 = r1.x + r1.w;
		let v2 = r2.x + r2.w;
		if (v1 > v2)
			r.w = v1 - r.x;
		else
			r.w = v2 - r.x;

		v1 = r1.y + r1.h;
		v2 = r2.y + r2.h;
		if (v1 > v2)
			r.h = v1 - r.y;
		else
			r.h = v2 - r.y;
			
		return r;
	}
}

class Dragger {
	constructor(x, y, label, image) {
		this.image = image;
		this.label = label;
		this.bounds = new Rectangle(x, y, image.width, image.height >> 1);
		this.anchor = {};
		this.state = 0;
	}
	contains(x, y) {
		return this.bounds.contains(x, y);
	}
	draw() {
		let image = this.image;
		let label = this.label;
		let x = this.bounds.x;
		let y = this.bounds.y;
		let width = this.bounds.w;
		let height = this.bounds.h;
		let sx = 0;
		let sy = (0 == this.state ? 0 : height);
		let color = (0 == this.state ? black : white);
		drawBackground();
		render.drawMasked(image, x, y, sx, sy, width, height, image.alpha, sx, sy);
		render.drawText(label, font, color,
			((width - render.getTextWidth(label, font)) >> 1) + x,
			((height - font.height) >> 1) + y
		);
	}
	onTouchBegan(x, y) {
		this.anchor.x = this.bounds.x - x;
		this.anchor.y = this.bounds.y - y;
		this.state = 1;
		this.update();
	}
	onTouchMoved(x, y) {
		let currentBounds = this.bounds;
		let newBounds = new Rectangle(this.anchor.x + x, this.anchor.y + y, this.bounds.w, this.bounds.h);
		let unionBounds = Rectangle.union(currentBounds, newBounds);
		this.bounds = newBounds;
		render.begin(unionBounds.x, unionBounds.y, unionBounds.w, unionBounds.h);
			this.draw();
		render.end();
	}
	onTouchEnded(x, y) {
		this.state = 0;
		this.update();
	}
	update() {
		render.begin(this.bounds.x, this.bounds.y, this.bounds.w, this.bounds.h);
			this.draw();
		render.end();
	}
}

let dragger = new Dragger((render.width - button.width) >> 1, (render.height - (button.height >> 1)) >> 1, "Drag Me", button);
let touch = new config.Touch;
touch.points = [{}];

render.begin();
	drawBackground();
	dragger.draw();
render.end();

Timer.repeat(() => {
	const points = touch.points;
	touch.read(points);
	const point = points[0];
	switch (point.state) {
		case 0:
		case 3:
			if (point.down) {
				delete point.down;
				point.target?.onTouchEnded(point.x, point.y);
				delete point.x;
				delete point.y;
				delete point.target;
			}
			break;
		case 1:
		case 2:
			if (!point.down) {
				point.down = true;
				point.target = dragger.contains(point.x, point.y) ? dragger : null;
				point.target?.onTouchBegan(point.x, point.y);
			}
			else
				point.target?.onTouchMoved(point.x, point.y);
			break;
	}
}, 17);
