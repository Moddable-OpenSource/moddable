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

assert.sameValue("3ad0da01a4416b89aa7c456481b7caa4", screen.checksum, "even align");

// even align blended 50%
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 8, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8, 128);
render.end();

assert.sameValue("d833736953ba283376342771a46dc1be", screen.checksum, "even align blended 50%");

// odd align
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 9, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("a6d5527b35cd7af4f76a4e34364130da", screen.checksum, "odd align");

// odd align blended 75%
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 9, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8, 192);
render.end();

assert.sameValue("5ec15b4233be088281989d3bf53b7497", screen.checksum, "odd align blended 75%");

// clipped left
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, -1, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("50c9c75579880526ea28c34735c937b1", screen.checksum, "clipped left");

// clipped right
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 32 - bitmap.width + 1, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("3970be785bc7fb9cc6e90b30a0a334e2", screen.checksum, "clipped right");

// clipped top
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, -1, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("1802fc558119f4da45a00dbb962ac43c", screen.checksum, "clipped top");

// clipped bottom
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, 32 - bitmap.height + 1, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("ac27bf58a8860471fcbc30162ad01cc6", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 8, 8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("5955b4b36bc5d0efb90d363d7e39850b", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 30, 30, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(0, 0, 1, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, 0, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("5a33142070e21cd4ea3a3955db89e613", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(0, 0, 32, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 0, 0, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("7a74f634bda0b941be253171ddd73187", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(0, 0, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, -8, -8, 0, 0, bitmap.width, bitmap.height, mask, 0, 8);
render.end();

assert.sameValue("85f939ef5bf69fd4e2b1ac2fac70bef8", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(0, 0, 32, 32);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMasked(bitmap, 10, 10, 0, 0, 8, 8, mask, 2, 2);
	render.drawMasked(bitmap, 19, 10, 8, 0, 8, 8, mask, 2, 2);
	render.drawMasked(bitmap, 10, 19, 0, 8, 8, 8, mask, 2, 2);
	render.drawMasked(bitmap, 19, 19, 8, 8, 8, 8, mask, 2, 2);
render.end();

assert.sameValue("a9f54fd44d5f48453572c6c2da803d12", screen.checksum, "source rect quarters");
