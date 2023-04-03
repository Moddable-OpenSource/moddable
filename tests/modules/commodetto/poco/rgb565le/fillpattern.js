/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");
assert((240 === screen.width) && (320 === screen.height), "unexpected screen");

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

assert.sameValue("12f003631e587a003ee8b6f1b74cf41e", screen.checksum, "even align");

// odd align
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 1, 0, 128, 128);
render.end();

assert.sameValue("9af59182831a313b800ea38f5d5e2446", screen.checksum, "odd align");

// clipped left & right
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, -bitmap.width / 2, 0, 128 + bitmap.width, 128);
render.end();

assert.sameValue("12f003631e587a003ee8b6f1b74cf41e", screen.checksum, "clipped left & right");

// clipped left & right cropped
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, -bitmap.width / 2, -2, 128 + bitmap.width, 128 + 2, 0, 0, bitmap.width / 2, bitmap.height / 2);
render.end();

assert.sameValue("6e19e2dccaebeff979b987761adbc3d7", screen.checksum, "clipped left & right cropped");

// clipped top & bottom
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, -bitmap.width / 2, 128, 128 + bitmap.width);
render.end();

assert.sameValue("f0684a728edbe8510ff2cbfcfa069288", screen.checksum, "top & bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("8ae20bde745dafd79e0f8ad890226b6e", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 30, 30, 128, 128);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(0, 0, 1, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("1be6e1ab3335de8d508b5634c54b9c91", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(0, 0, 128, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("19fd7d8d7fac49d464a5446ebfb75e5a", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 0, 0, 128, 128);
render.end();

assert.sameValue("e8520647df23f3e03a54cb51778c781c", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(0, 0, 128, 128);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillPattern(bitmap, 10, 10, 8, 8, 0, 0, 8, 8);
	render.fillPattern(bitmap, 19, 10, 8, 8, 8, 0, 8, 8);
	render.fillPattern(bitmap, 10, 19, 8, 8, 0, 8, 8, 8);
	render.fillPattern(bitmap, 19, 19, 8, 8, 8, 8, 8, 8);
render.end();

assert.sameValue("97f5103f90c74ba6c67e48b84199972d", screen.checksum, "source rect quarters");
