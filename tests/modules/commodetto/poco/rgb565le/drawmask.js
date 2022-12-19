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
const bitmap = new Bitmap(16, 16, Bitmap.Gray16, pixels.buffer, 4);
pixels.fill(red, 0, 16 * 4);
pixels.fill(green, 4 * 16, 8 * 16);
pixels.fill(blue, 8 * 16, 12 * 16);
pixels.fill(white, 12 * 16, 16 * 16);

const pixels8 = new Uint8Array(8 * 32);
const mask = new Bitmap(16, 32, Bitmap.Gray16, pixels8.buffer, 0);
for (let y = 0; y < 32; y++) {
	const g = y >> 1;
	pixels8.fill(g | (g << 4), y * 8, (y + 1) * 8);
}

// even align
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 8, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("fddd631a5c2538b9a446c66eedf1f176", screen.checksum, "even align");

// even align blended 50%
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 8, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8, 128);
render.end();

assert.sameValue("6e725724b8fdea6a343418aefb1682c4", screen.checksum, "even align blended 50%");

// odd align
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 9, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("d3151a4289d4c2ab82070ea7cba6a5e9", screen.checksum, "odd align");

// odd align blended 75%
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 9, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8, 192);
render.end();

assert.sameValue("f3cf0a7fdf0d6fe585e390fc8285c78f", screen.checksum, "odd align blended 75%");

// clipped left
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, -1, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("116604acf2cf5e017d4007127d6895d7", screen.checksum, "clipped left");

// clipped right
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 32 - bitmap.width + 1, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("f463d1f16c4a0e4a54267dc56374404e", screen.checksum, "clipped right");

// clipped top
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, -1, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("08a303e5cca984a806a5eaeb8ad1b193", screen.checksum, "clipped top");

// clipped bottom
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, 32 - bitmap.height + 1, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("c951b33e78dcb72e4a61390d1584aa75", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 8, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("a91b205660f8b6e1379a3ee837eb9de5", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 30, 30, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(0, 0, 1, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, 0, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("e91111c78263ea8dbe0590258f190c6e", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(0, 0, 32, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, 0, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("6a7cd824ccd17c1928cecc799f489339", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(0, 0, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, -8, -8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("5be0b4ca97f7fba6aec316ab2e85cd48", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 10, 10, 0, 0, 8, 8, mask, 2, 2);
	render.drawMasked(bitmap, 19, 10, 8, 0, 8, 8, mask, 2, 2);
	render.drawMasked(bitmap, 10, 19, 0, 8, 8, 8, mask, 2, 2);
	render.drawMasked(bitmap, 19, 19, 8, 8, 8, 8, mask, 2, 2);
render.end();

assert.sameValue("067a2c87ba9b65241922fe2e11255122", screen.checksum, "source rect quarters");
