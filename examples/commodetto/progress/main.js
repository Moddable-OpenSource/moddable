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
/*
 *     Spinner graphic from KPR sample code Open Source repository:
 *     https://github.com/Kinoma/KPR-examples/blob/master/flickr-grid/src/mobile/assets/spinner-strip-80px-24cell-blue.png
 */

import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(screen);

const white = render.makeColor(255, 255, 255);
const green = render.makeColor(0, 255, 0);
const red = render.makeColor(255, 0, 0);
const backgroundColor = render.makeColor(0, 0, 0);

let titleFont = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
let progressFont = parseBMF(new Resource("OpenSans-Semibold-28.bf4"));
let spinner = parseRLE(new Resource("spinner-strip-80px-24cell-blue-alpha.bm4"));

const title = "Progress!";
const headerHeight = 30;
const barMargin = 20;
const barThickness = 20;

class Progress {
	constructor(dictionary) {
		this.x = dictionary.x;
		this.y = dictionary.y;
		this.width = dictionary.width;
		this.height = dictionary.height;
		this.backgroundColor = backgroundColor;
	}
	update(percent) {
		debugger;
	}
}

class ProgressBar extends Progress {
	constructor(dictionary) {
		super(dictionary);
		this.frameColor = dictionary.frameColor;
		this.fillColor = dictionary.fillColor;
	}
	update(percent) {
		let backgroundColor = this.backgroundColor;
		let frameColor = this.frameColor;
		render.fillRectangle(backgroundColor, this.x + 1, this.y + 1, this.width - 1, this.height - 1);
		render.fillRectangle(frameColor, this.x, this.y, this.width, 1);
		render.fillRectangle(frameColor, this.x, this.y, 1, this.height);
		render.fillRectangle(frameColor, this.x, this.y + this.height - 1, this.width - 1, 1);
		render.fillRectangle(frameColor, this.x + this.width - 1, this.y, 1, this.height);
	}
}

class HorizontalProgressBar extends ProgressBar {
	constructor(dictionary) {
		super(dictionary);
	}
	update(percent) {
		render.begin(this.x, this.y, this.width, this.height);
			super.update(percent);
			render.fillRectangle(this.fillColor, this.x + 1, this.y + 1, this.width * (percent / 100), this.height - 2);
		render.end();
	}
}

class VerticalProgressBar extends ProgressBar {
	constructor(dictionary) {
		super(dictionary);
	}
	update(percent) {
		render.begin(this.x, this.y, this.width, this.height);
			super.update(percent);
			render.fillRectangle(this.fillColor, this.x + 1, this.y + 1, this.width - 2, this.height * (percent / 100));
		render.end();
	}
}

class Spinner extends Progress {
	constructor(dictionary) {
		super(dictionary);
		this.cells = dictionary.cells;
		this.bitmap = dictionary.bitmap;
		this.color = render.makeColor(49, 101, 173);
		this.cell = 0;
		this.count = 0;
	}
	update(percent) {
		let sx = this.cell * this.width;
		render.begin(this.x, this.y, this.width, this.height);
			render.fillRectangle(this.backgroundColor, this.x, this.y, this.width, this.height);
			render.drawGray(this.bitmap, this.color, this.x, this.y, sx, 0, this.width, this.height);
		render.end();
		this.count += 1;
		if (3 == this.count) {
			this.count = 0;
			this.cell += 1;
			if (this.cells == this.cell)
				this.cell = 0;
		}
	}
}

class TextProgress extends Progress {
	constructor(dictionary) {
		super(dictionary);
		this.font = dictionary.font;
		this.fillColor = dictionary.fillColor;
	}
	update(percent) {
		let text = percent + "%";
		let width = render.getTextWidth(text, this.font);
		render.begin(this.x, this.y, this.width, this.height);
			render.fillRectangle(this.backgroundColor, this.x, this.y, this.width, this.height);
			render.drawText(text, this.font, this.fillColor, (render.width - width) >> 1, this.y);
		render.end();
	}
}

let progressWidth = render.getTextWidth("100%", progressFont);	// maximum width

let indicators = [
	new HorizontalProgressBar({
		x:barMargin, y:render.height - barMargin - barThickness, width:render.width - (barMargin << 1), height:barThickness,
		fillColor:green, frameColor:white
	}),
	new VerticalProgressBar({
		x:barMargin, y:headerHeight + barMargin, width:barThickness, height:render.height - (barMargin << 1) - (3 * barThickness),
		fillColor:red, frameColor:white
	}),
	new Spinner({
		x:(render.width - 80) >> 1, y:render.height - barThickness - (barMargin * 3) - 80, width:80, height:80,
		bitmap:spinner, cells:24
	}),
	new TextProgress({
		x:(render.width - progressWidth) >> 1, y:(render.height - (progressFont.height << 2)) >> 1, width:progressWidth, height:progressFont.height,
		font:progressFont,
		fillColor:white
	}),
];

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
	render.drawText(title, titleFont, white, (render.width - render.getTextWidth(title, titleFont)) / 2, (headerHeight - titleFont.height) / 2);
	render.fillRectangle(white, 0, headerHeight + 2, render.width, 1);
render.end();

let percent = 0;
let bump = +1;

Timer.repeat(() => {
	indicators.forEach(indicator => {
		indicator.update(percent);
	});
	percent += bump;
	if (percent > 100) {
		percent = 99;
		bump = -1;
	}
	else if (percent < 0) {
		percent = 1;
		bump = +1;
	}
}, 25);
