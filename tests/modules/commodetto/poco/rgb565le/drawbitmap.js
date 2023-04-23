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
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("522e0e8d05b7feb443fdc4234a671f31", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 11, 10);
render.end();

assert.sameValue("6e6fba2ec58ce902d29939baa67cdab0", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 9, 10);
render.end();

assert.sameValue("4d7cef59b83db123ac052461272d7ee6", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 15, 10);
render.end();

assert.sameValue("f416ebaa2b0fa369cde10ecfb349fafe", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 9);
render.end();

assert.sameValue("6641e623d611d17ef79c5de84e32bd6f", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 15);
render.end();

assert.sameValue("687ca0b87a2f9ca685ee4594e0321e68", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("68ae548975da70afc3ac6654d2b63662", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, -10, -10);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("676298115dd2141c2fadaf970abf2885", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("5db625d0b288696411a09a4592fe9938", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("2d8b7393b71b19f3e49a66bfe34aeb7b", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10, 0, 0, 8, 8);
	render.drawBitmap(bitmap, 19, 10, 8, 0, 8, 8);
	render.drawBitmap(bitmap, 10, 19, 0, 8, 8, 8);
	render.drawBitmap(bitmap, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("66e98e50383234324b0990edf8912ca6", screen.checksum, "source rect quarters");
