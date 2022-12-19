/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");

const render = new Poco(screen);

const black = render.makeColor(0, 0, 0);
const white = render.makeColor(255, 255, 255);
const red = render.makeColor(255, 0, 0);
const green = render.makeColor(0, 255, 0);
const blue = render.makeColor(0, 0, 255);

const pixels = new Uint16Array(new ArrayBuffer(16 * 16 * 2 + 4), 4);
const bitmap = new Bitmap(16, 16, Bitmap.Default, pixels.buffer, 4);
pixels.fill(red, 0, 16 * 4);
pixels.fill(green, 4 * 16, 8 * 16);
pixels.fill(blue, 8 * 16, 12 * 16);
pixels.fill(white, 12 * 16, 16 * 16);

// even align
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("87c71d5412add2a934b318f3c7b17a76", screen.checksum, "even align");

// odd align
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 1, 0, 128, 128);
render.end();

assert.sameValue("d6ea64065f1f0a0bf452704c6437e88e", screen.checksum, "odd align");

// clipped left & right
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, -bitmap.width / 2, 0, 128 + bitmap.width, 128);
render.end();

assert.sameValue("87c71d5412add2a934b318f3c7b17a76", screen.checksum, "clipped left & right");

// clipped left & right cropped
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, -bitmap.width / 2, -2, 128 + bitmap.width, 128 + 2, 0, 0, bitmap.width / 2, bitmap.height / 2);
render.end();

assert.sameValue("ff75768115cf0dd48f6aedb2caad9066", screen.checksum, "clipped left & right cropped");

// clipped top & bottom
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, -bitmap.width / 2, 128, 128 + bitmap.width);
render.end();

assert.sameValue("9a74ea839825f56ea737abc99df8a631", screen.checksum, "top & bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("a94556abe963e91c5b41606374aaf7e1", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 30, 30, 128, 128);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(0, 0, 1, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("3825d72f87d3ad5ce652687f666ef631", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(0, 0, 128, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("c69197d26af11bc728f8cf7df698328c", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("a0497bec042a6c2f536d8bf250f4d095", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 10, 10, 8, 8, 0, 0, 8, 8);
	render.fillPattern(bitmap, 19, 10, 8, 8, 8, 0, 8, 8);
	render.fillPattern(bitmap, 10, 19, 8, 8, 0, 8, 8, 8);
	render.fillPattern(bitmap, 19, 19, 8, 8, 8, 8, 8, 8);
render.end();

assert.sameValue("ae09a393a0d4be96665b7e94956ca601", screen.checksum, "source rect quarters");
