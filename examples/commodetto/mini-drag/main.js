/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import config from "mc/config";
import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";

let render = new Poco(global.screen ? screen : new (require(config.screen))({}));

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);

let font = parseBMF(new Resource("OpenSans-Regular-20.bf4"));
let background = parseBMP(new Resource("desktop-color.bmp"));
let button = parseBMP(new Resource("button-color.bmp"));
button.alpha = parseBMP(new Resource("button-alpha.bmp"));

function drawBackground() {
	render.fillPattern(background, 0, 0, render.width, render.height, 0, 0, background.width, background.height);
}

class Dragger {
	constructor(x, y, label, image) {
		this.image = image;
		this.label = label;
		this.width = image.width;
		this.height = image.height >> 1;
		this.position = { x: x - (this.width >> 1), y: y - (this.height >> 1) };
		this.anchor = {};
		this.state = 0;
	}
	contains(x, y) {
		if (x >= this.position.x && x <= this.position.x + this.width && y >= this.position.y && y <= this.position.y + this.height)
			return true;
		return false;
	}
	draw() {
		let image = this.image;
		let label = this.label;
		let x = this.position.x;
		let y = this.position.y;
		let sx = 0;
		let sy = (0 == this.state ? 0 : this.height);
		let color = (0 == this.state ? black : white);
		drawBackground();
		render.drawMasked(image, x, y, sx, sy, this.width, this.height, image.alpha, sx, sy);
		render.drawText(label, font, color,
			((this.width - render.getTextWidth(label, font)) >> 1) + x,
			((this.height - font.height) >> 1) + y
		);
	}
	onTouchBegan(x, y) {
		this.anchor.x = this.position.x - x;
		this.anchor.y = this.position.y - y;
		this.state = 1;
		this.update();
	}
	onTouchMoved(x, y) {
		let currentPosition = this.position;
		let newPosition = { x:this.anchor.x + x, y:this.anchor.y + y };
		let invalX, invalY, invalWidth, invalHeight;
		if (currentPosition.x < newPosition.x) {
			invalX = currentPosition.x;
			invalWidth = newPosition.x + this.width;
		}
		else {
			invalX = newPosition.x;
			invalWidth = currentPosition.x + this.width;
		}
		if (currentPosition.y < newPosition.y) {
			invalY = currentPosition.y;
			invalHeight = newPosition.y + this.height;
		}
		else {
			invalY = newPosition.y;
			invalHeight = currentPosition.y + this.height;
		}
		this.position = newPosition;
		render.begin(invalX, invalY, invalWidth, invalHeight);
			this.draw();
		render.end();
	}
	onTouchEnded(x, y) {
		this.state = 0;
		this.update();
	}
	update() {
		render.begin(this.position.x, this.position.y, this.width, this.height);
			this.draw();
		render.end();
	}
}

let dragger = new Dragger(render.width >> 1, render.height >> 1, "Drag Me", button);
let touch = require(config.touch);
touch = new touch;
touch.points = [{}];

render.begin();
	drawBackground();
	dragger.draw();
render.end();

let timer = Timer.repeat(() => {
	let points = touch.points;
	touch.read(points);
	let point = points[0];
	switch (point.state) {
		case 0:
		case 3:
			if (point.down) {
				delete point.down;
				dragger.onTouchEnded(point.x, point.y);
				delete point.x;
				delete point.y;
			}
			break;
		case 1:
			if (dragger.contains(point.x, point.y) && !point.down) {
				point.down = true;
				dragger.onTouchBegan(point.x, point.y);
			}
			break;
		case 2:
			dragger.onTouchMoved(point.x, point.y);
			break;
	}
}, 17);
