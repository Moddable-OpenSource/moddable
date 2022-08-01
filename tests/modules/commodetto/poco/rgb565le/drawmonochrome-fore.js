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
	foreground only
*/

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, undefined, 10, 10);
render.end();

assert.sameValue("e784f0506a604703f32ae646556fdc62", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, undefined, 11, 10);
render.end();

assert.sameValue("8c27cc425722104963e43a911393d2fb", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, 9, 10);
render.end();

assert.sameValue("bef5a260a2abd99a17afa984bee3f096", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, undefined, 15, 10);
render.end();

assert.sameValue("a9a9681c1304f2b451e57f572549af61", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, undefined, 10, 9);
render.end();

assert.sameValue("d81c644c75e5cc57230bfba6c23789a6", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, undefined, 10, 15);
render.end();

assert.sameValue("2d8ef957688e35671d4305063082c080", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, undefined, 10, 10);
render.end();

assert.sameValue("069360d446b22c4bc66d6db8cb7e0a67", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, -10, -10);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, undefined, 10, 10);
render.end();

assert.sameValue("68849fcc054480b20541f58ede6801a2", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, 10, 10);
render.end();

assert.sameValue("f40456fa2fa9057d9d6ec56c4f6eb33a", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, undefined, 10, 10);
render.end();

assert.sameValue("a0497bec042a6c2f536d8bf250f4d095", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, 10, 10, 0, 0, 8, 8);
	render.drawMonochrome(bitmap, white, undefined, 19, 10, 8, 0, 8, 8);
	render.drawMonochrome(bitmap, blue, undefined, 10, 19, 0, 8, 8, 8);
	render.drawMonochrome(bitmap, green, undefined, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("a160c1dad33f0782fcee75d50d6d5af1", screen.checksum, "source rect quarters");
