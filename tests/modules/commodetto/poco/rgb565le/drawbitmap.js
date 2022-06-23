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
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("2122b6ae4603b65d98939ff9f3ef78a6", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 11, 10);
render.end();

assert.sameValue("2fc5d5cb8ff0a0dbc4b77b39c4265eb3", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 9, 10);
render.end();

assert.sameValue("28c471ad7cc502e18023aa859374469e", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 15, 10);
render.end();

assert.sameValue("6be3188c83542a7e6a35d95708ad8ac2", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 9);
render.end();

assert.sameValue("fca73840e3076ebc60585b7551b51394", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 15);
render.end();

assert.sameValue("2d15cd73f86ad0ad47a87c686da4ddd2", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("ba57c32c37142d5b64a6c52fd08d91b1", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, -10, -10);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("e8acff737d39a9ee56d819edb0407363", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("9e2e5e9a23f3b8f5a6e51ab1fcbed21c", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10);
render.end();

assert.sameValue("fe36ebbc617b0193f6d60af23e636082", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawBitmap(bitmap, 10, 10, 0, 0, 8, 8);
	render.drawBitmap(bitmap, 19, 10, 8, 0, 8, 8);
	render.drawBitmap(bitmap, 10, 19, 0, 8, 8, 8);
	render.drawBitmap(bitmap, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("14777c7290cf61a32c67a57bda6b3643", screen.checksum, "source rect quarters");
