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

const pixels = new Uint8Array(new ArrayBuffer(16 * 16 / 2 + 4), 4);
const bitmap = new Bitmap(16, 16, Bitmap.Gray16, pixels.buffer, 4);
pixels.fill(0x00, 0, 16 * 2);
pixels.fill(0x22, 2 * 16, 4 * 16);
pixels.fill(0x44, 4 * 16, 6 * 16);
pixels.fill(0x66, 6 * 16, 8 * 16);

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10);
render.end();

assert.sameValue("d80a41fd6392caac83227a0178fd3454", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 11, 10);
render.end();

assert.sameValue("9efae1cb726c7872154eb357153fe396", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 9, 10);
render.end();

assert.sameValue("e5a8d14a4d5d66cc58cdc3c2181e1d70", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, 15, 10);
render.end();

assert.sameValue("1a9dd7b51f8fa348a321911b33832b6a", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 9);
render.end();

assert.sameValue("85c427c97824ef3a07ca9ce6b5a938d1", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 15);
render.end();

assert.sameValue("cebde48b23d5980762dc2586afdd19c1", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10, 10);
render.end();

assert.sameValue("4a4835a1a70c0a4c8df103a16c32cd90", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, -10, -10);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10);
render.end();

assert.sameValue("920ce1eff256036e9496c9dd0f17e21a", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 10);
render.end();

assert.sameValue("5db625d0b288696411a09a4592fe9938", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10, 10);
render.end();

assert.sameValue("faf82d36a6a38b5f4e34f3bb8f5136e5", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10, 0, 0, 8, 8);
	render.drawGray(bitmap, red, 19, 10, 8, 0, 8, 8);
	render.drawGray(bitmap, green, 10, 19, 0, 8, 8, 8);
	render.drawGray(bitmap, blue, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("b70e3db1f51110c74bc9f158857e13dd", screen.checksum, "source rect quarters");
