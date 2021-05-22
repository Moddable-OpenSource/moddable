/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import BufferOut from "commodetto/BufferOut";
import JPEG from "commodetto/readJPEG";
import loadJPEG from "commodetto/loadJPEG";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";

let poco = new Poco(screen);
let white = poco.makeColor(255, 255, 255);
let black = poco.makeColor(0, 0, 0);
let gray = poco.makeColor(128, 128, 128);
let red = poco.makeColor(255, 0, 0);
let green = poco.makeColor(0, 255, 0);
let blue = poco.makeColor(0, 0, 255);

function fillRectangle() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
	poco.fillRectangle(red, 0, 0, poco.width / 2, poco.height);
	poco.blendRectangle(blue, 128, poco.width / 4,
						0, poco.width / 2, poco.height);
}

function origin() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);

	poco.origin(10, 10);
	poco.fillRectangle(red, 0, 0, 40, 20);

	poco.origin(25, 25);
	poco.fillRectangle(green, 0, 0, 40, 20);

	poco.origin(25, 25);
	poco.fillRectangle(blue, 0, 0, 40, 20);

	poco.blendRectangle(black, 128, -4, -4, 20, 10);
	poco.origin();

	poco.blendRectangle(black, 128, -4, -4, 20, 10);
	poco.origin();

	poco.blendRectangle(black, 128, -4, -4, 20, 10);
	poco.origin();
}

function clip() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);

	poco.clip(20, 20, poco.width - 40, poco.height - 40);
	poco.fillRectangle(green, 0, 0, poco.width, poco.height);

	poco.clip(0, 0, 40, 40);
	poco.fillRectangle(blue, 0, 0, poco.width, poco.height);

	poco.fillRectangle(white, 26, 0, 2, poco.height);

	poco.clip();
	poco.fillRectangle(red, 30, 0, 2, poco.height);

	poco.clip();
	poco.fillRectangle(black, 34, 0, 2, poco.height);
}

function monochrome() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);

	let envelope = parseBMP(new Resource("envelope.bmp"));
	poco.drawMonochrome(envelope, black, white, 14, 10)
	poco.drawMonochrome(envelope, red, white, 14, 55)
	poco.drawMonochrome(envelope, green, undefined, 74, 10)
	poco.drawMonochrome(envelope, undefined, blue, 74, 55)
}

function colorBitmap() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);

	let image = parseBMP(new Resource("lvb.bmp"));

	let x = 0;
	let y = Math.round((poco.height - image.height) / 2);
	poco.drawBitmap(image, x, y);

	x = image.width;
	poco.drawBitmap(image, x + 25, y + 38, 25, 38, 11, 7);   // left eye
	poco.drawBitmap(image, x +  7, y + 40,  7, 40, 10, 6);   // right eye
	poco.drawBitmap(image, x + 15, y + 56, 15, 56, 16, 6);   // mouth
}

function pattern() {
	let pattern = parseBMP(new Resource("pattern1.bmp"));
	poco.fillPattern(pattern, 0, 0, poco.width, poco.height);
	poco.fillPattern(pattern, 28, 28, 63, 35, 21, 14, 7, 7);
}

function grayBitmap() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);

	let image = parseBMP(new Resource("envelope-gray.bmp"));

	poco.drawGray(image, black, 10, 2);
	poco.drawGray(image, white, 10, 47);

	poco.drawGray(image, black, 70, 2);
	poco.drawGray(image, green, 70 + 2, 2 + 2);

	poco.drawGray(image, white, 70, 47);
	poco.drawGray(image, red, 70 + 2, 47 + 2);
}

function offscreen() {
	let offscreen = new BufferOut({width: 30, height: 30, pixelFormat: poco.pixelsOut.pixelFormat});
	let pocoOff = new Poco(offscreen);
	pocoOff.begin();
		pocoOff.fillRectangle(gray, 0, 0, 30, 30);
		pocoOff.fillRectangle(red, 2, 2, 26, 26);
		pocoOff.fillRectangle(black, 4, 4, 22, 22);
		pocoOff.fillRectangle(blue, 6, 6, 18, 18);
		pocoOff.fillRectangle(white, 8, 8, 14, 14);
		pocoOff.fillRectangle(green, 10, 10, 10, 10);
		pocoOff.fillRectangle(gray, 13, 13, 4, 4);
	pocoOff.end();

	poco.fillPattern(offscreen.bitmap, 0, 0, poco.width, poco.height);
}

function alpha() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);

	let girl = parseBMP(new Resource("girl.bmp"));
	let circle = parseBMP(new Resource("mask_circle.bmp"));
	let square = parseBMP(new Resource("mask_square.bmp"));

	poco.drawBitmap(girl, 0, 2);
	poco.drawGray(circle, black, 40, 2);
	poco.drawMasked(girl, 80, 2, 0, 0,
					circle.width, circle.height, circle, 0, 0);

	poco.drawBitmap(girl, 0, 47);
	poco.drawGray(square, black, 40, 47);
	poco.drawMasked(girl, 80, 47, 0, 0,
					square.width, square.height, square, 0, 0);
}

function jpeg() {
	if (0) {
		/* requires more memory than many MCUs have free */
		let piano = loadJPEG(new Resource("piano.jpg"));
		trace(`width ${piano.width}, height ${piano.height}\n`);

		poco.begin()
			poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
			poco.drawBitmap(piano, 0, 0);
		poco.end();
	}
	else {
		poco.begin()
			poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
		poco.end();

		let jpeg = new JPEG(new Resource("piano.jpg"));
		let block;
		while (block = jpeg.read()) {
			poco.begin(block.x, block.y, block.width, block.height);
			poco.drawBitmap(block, block.x, block.y);
			poco.end();
		}
	}
}

function text1() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
	poco.fillRectangle(white, 2, 2, poco.width - 4, poco.height - 4);

	let palatino36 = parseBMF(new Resource("palatino_36.fnt"));
	palatino36.bitmap = parseBMP(new Resource("palatino_36.bmp"));

	poco.drawText("Hello.", palatino36, black, 4, 20);
	poco.drawText("Hello.", palatino36, green, 4, 55);
}

function text2() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
	poco.fillRectangle(white, 2, 2, poco.width - 4, poco.height - 4);

	let palatino36 = parseBMF(new Resource("palatino_36.fnt"));
	palatino36.bitmap = parseBMP(new Resource("palatino_36.bmp"));

	poco.drawText("Hello, world. This is long.", palatino36, red, 2, 10);
	poco.drawText("Hello, world. This is long.", palatino36, green, 2, 45, poco.width - 2);
}

function text3() {
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
	poco.fillRectangle(white, 2, 2, poco.width - 4, poco.height - 4);

	let palatino12 = parseBMF(new Resource("OpenSans-SemiboldItalic-18.fnt"));
	palatino12.bitmap = parseBMP(new Resource("OpenSans-SemiboldItalic-18.bmp"));

	poco.drawText("T Left", palatino12, red,
				  2, 2);
	poco.drawText("T Right", palatino12, green,
				  poco.width - 2 - poco.getTextWidth("T Right", palatino12), 2);

	poco.drawText("B Left", palatino12, blue,
				  2, poco.height - 2 - palatino12.height);
	poco.drawText("B Right", palatino12, gray,
				  poco.width - 2 - poco.getTextWidth("B Right", palatino12),
				  poco.height - 2 - palatino12.height);

	poco.drawText("Centered", palatino12, black,
				  (poco.width - poco.getTextWidth("Centered", palatino12)) / 2,
				  (poco.height - palatino12.height) / 2);
}

function text4() {
	poco.fillRectangle(green, 0, 0, screen.width, screen.height);

	let openSans52 = parseBMF(new Resource("OpenSans-BoldItalic-52.fnt"));
	openSans52.bitmap = parseBMP(new Resource("OpenSans-BoldItalic-52-color.bmp"));
	openSans52.mask = parseBMP(new Resource("OpenSans-BoldItalic-52-alpha.bmp"));

	poco.drawText("Poco", openSans52, openSans52.mask, 0, 5);
}

let examples = [fillRectangle, origin, clip, monochrome, pattern, grayBitmap, offscreen, alpha, jpeg, text1, text2, text3, text4];
let index = 0;

Timer.repeat(() => {
	 if (jpeg == examples[index])
		 examples[index](poco);
	 else {
		poco.begin();
			examples[index](poco);
		poco.end();
	}
	index = (index + 1) % examples.length;
}, 500, 0);
