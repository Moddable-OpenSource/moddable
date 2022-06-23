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

const pixels = new Uint8Array(new ArrayBuffer(4 * 16 + 4), 4);
const bitmap = new Bitmap(32, 16, Bitmap.Monochrome, pixels.buffer, 4);
pixels.fill(0xF0, 0, 4 * 4);
pixels.fill(0xAA, 4 * 4, 8 * 4);
pixels.fill(0x0F, 8 * 4, 12 * 4);
pixels.fill(0x55, 12 * 4, 4 * 16);

/*
	background only
*/

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, 10, 10);
render.end();

assert.sameValue("b7d3971098d06bfd570132241cb6bb7f", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, blue, 11, 10);
render.end();

assert.sameValue("85d05a7d3511f23aedc07480063e5a12", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, blue, 9, 10);
render.end();

assert.sameValue("c5bf158572177223e40b4fe9db5ad46e", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, green, 15, 10);
render.end();

assert.sameValue("0fdcf901865663c7774e9642e6fff5a0", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 9);
render.end();

assert.sameValue("dd6629902d17762c947f0714554abfd8", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, 10, 15);
render.end();

assert.sameValue("2d1cb049639372af09f0f17ed4be3fab", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, green, 10, 10);
render.end();

assert.sameValue("faf9fc837d1daa95656ed510b988100c", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, -10, -10);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, 10, 10);
render.end();

assert.sameValue("00e2f28e6011bd396aa9552743fc10f3", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 10);
render.end();

assert.sameValue("707adf497d4a32ca0dc1c9b94dce12e5", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 10);
render.end();

assert.sameValue("c183857770364b05c2011bdebb914ed3", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 10, 0, 0, 8, 8);
	render.drawMonochrome(bitmap, undefined, blue, 19, 10, 8, 0, 8, 8);
	render.drawMonochrome(bitmap, undefined, green, 10, 19, 0, 8, 8, 8);
	render.drawMonochrome(bitmap, undefined, red, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("899bfaa8611d274000388b07e682baca", screen.checksum, "source rect quarters");
