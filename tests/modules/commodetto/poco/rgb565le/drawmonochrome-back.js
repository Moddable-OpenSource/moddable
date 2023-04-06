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
	background only
*/

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, 10, 10);
render.end();

assert.sameValue("4b7cf7274140df8bcfd4fa7ba7747ea4", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, blue, 11, 10);
render.end();

assert.sameValue("42c7ab127547e75ace08c97213f15e20", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, blue, 9, 10);
render.end();

assert.sameValue("751608640c5bd69f279a6c953b01744b", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, green, 15, 10);
render.end();

assert.sameValue("b57c3103fafd578905063cfe0e038e93", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 9);
render.end();

assert.sameValue("1c36b5ebc75a010f6671c96de23d7894", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, 10, 15);
render.end();

assert.sameValue("7f59d3481f7275120b9d6b8f2a99f3b6", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, green, 10, 10);
render.end();

assert.sameValue("218d9d6b87e701aa15cc9551f84ae38d", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, -10, -10);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, red, 10, 10);
render.end();

assert.sameValue("cd9ef213621dd024f0d30c16737b6b6d", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 10);
render.end();

assert.sameValue("8c847e6d3885e02e4864c5a3c466dc39", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 10);
render.end();

assert.sameValue("2fa4f2ef4ecf6f2ebb8215aa9eadb18c", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, undefined, white, 10, 10, 0, 0, 8, 8);
	render.drawMonochrome(bitmap, undefined, blue, 19, 10, 8, 0, 8, 8);
	render.drawMonochrome(bitmap, undefined, green, 10, 19, 0, 8, 8, 8);
	render.drawMonochrome(bitmap, undefined, red, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("aa3b33ab59b35535588932622bdeb3a0", screen.checksum, "source rect quarters");
