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

const pixels = new Uint8Array(new ArrayBuffer(4 * 16 + 4), 4);
const bitmap = new Bitmap(32, 16, Bitmap.Monochrome, pixels.buffer, 4);
pixels.fill(0xF0, 0, 4 * 4);
pixels.fill(0xAA, 4 * 4, 8 * 4);
pixels.fill(0x0F, 8 * 4, 12 * 4);
pixels.fill(0x55, 12 * 4, 4 * 16);

/*
	both colors
*/

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, red, 10, 10);
render.end();

assert.sameValue("46caf035515e075d7f73e8057729f6cb", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, blue, 11, 10);
render.end();

assert.sameValue("8410e9b8a3e6aecc819a6ba4b111c187", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, blue, 9, 10);
render.end();

assert.sameValue("8aeafe3368ee74604bd306cd05dfd1e8", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, green, 15, 10);
render.end();

assert.sameValue("f76d27ad41e382af0e229532160b874a", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, white, 10, 9);
render.end();

assert.sameValue("a821a2a2941fede6d1bf837fbb75ae7e", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, red, 10, 15);
render.end();

assert.sameValue("7c07dd18d2ce9533107163ac895499cf", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, green, 10, 10);
render.end();

assert.sameValue("b08e9bd312514ba9795d8c33572e6c8e", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, red, -10, -10);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, red, 10, 10);
render.end();

assert.sameValue("22d40f5fb689d58867b1486767018017", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, white, 10, 10);
render.end();

assert.sameValue("5ccfb22e32f7d408b672c2aea5c39f4d", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, white, 10, 10);
render.end();

assert.sameValue("e8520647df23f3e03a54cb51778c781c", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, white, 10, 10, 0, 0, 8, 8);
	render.drawMonochrome(bitmap, white, blue, 19, 10, 8, 0, 8, 8);
	render.drawMonochrome(bitmap, blue, green, 10, 19, 0, 8, 8, 8);
	render.drawMonochrome(bitmap, green, red, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("427dfc791bfdf340368f661d649e3103", screen.checksum, "source rect quarters");
