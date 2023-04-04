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
	foreground only
*/

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, undefined, 10, 10);
render.end();

assert.sameValue("592d60d594d16fed962854048e0797b8", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, undefined, 11, 10);
render.end();

assert.sameValue("dfa4f8aaa234276cab44a0bc0bd9d45c", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, 9, 10);
render.end();

assert.sameValue("390fd968a79780b84f30de7f184be631", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, undefined, 15, 10);
render.end();

assert.sameValue("72f2642d949f019a8eb2f57370202c89", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, undefined, 10, 9);
render.end();

assert.sameValue("35712e6e00123b51999bc516fc602d3b", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, undefined, 10, 15);
render.end();

assert.sameValue("9104fc24588c1b759208df81594f43bd", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, undefined, 10, 10);
render.end();

assert.sameValue("e38f6b761bf03836bcee072ddd4517f0", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, -10, -10);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, undefined, 10, 10);
render.end();

assert.sameValue("c22345dcd8b3210fced75861f53d8453", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, 10, 10);
render.end();

assert.sameValue("59b460d0a2dd00cbcc04433eadfa2f34", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, undefined, 10, 10);
render.end();

assert.sameValue("e8520647df23f3e03a54cb51778c781c", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, undefined, 10, 10, 0, 0, 8, 8);
	render.drawMonochrome(bitmap, white, undefined, 19, 10, 8, 0, 8, 8);
	render.drawMonochrome(bitmap, blue, undefined, 10, 19, 0, 8, 8, 8);
	render.drawMonochrome(bitmap, green, undefined, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("a39dfaa417fa5533cddc856701f6b7b0", screen.checksum, "source rect quarters");
