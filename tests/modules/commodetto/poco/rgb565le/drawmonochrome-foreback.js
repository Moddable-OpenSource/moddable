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
	both colors
*/

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, red, 10, 10);
render.end();

assert.sameValue("6937fdbe7dba788d0a6ea4fb4e6d6160", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, blue, 11, 10);
render.end();

assert.sameValue("3a920dc5188a72bd1ebeeca2600dae8e", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, blue, 9, 10);
render.end();

assert.sameValue("f687c3c4df84ae56fa5fdc7deaafb4ae", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, green, 15, 10);
render.end();

assert.sameValue("d4001fb07ea6c5355e5aa903a335c9cf", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, green, white, 10, 9);
render.end();

assert.sameValue("355542e76ffb01f0584d927832b3ff3f", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, red, 10, 15);
render.end();

assert.sameValue("01dd5582c5db2fa31778eadda0a24ec5", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, green, 10, 10);
render.end();

assert.sameValue("28cab108e5881c074f4adb8ceaffceab", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, red, -10, -10);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, white, red, 10, 10);
render.end();

assert.sameValue("0275609e8fc3614f67e197ad063dfef3", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, white, 10, 10);
render.end();

assert.sameValue("4b6468a9d0fe7fd6f9f29f3d1d3e6bd3", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, blue, white, 10, 10);
render.end();

assert.sameValue("a0497bec042a6c2f536d8bf250f4d095", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawMonochrome(bitmap, red, white, 10, 10, 0, 0, 8, 8);
	render.drawMonochrome(bitmap, white, blue, 19, 10, 8, 0, 8, 8);
	render.drawMonochrome(bitmap, blue, green, 10, 19, 0, 8, 8, 8);
	render.drawMonochrome(bitmap, green, red, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("6a97ba04c2e87c2e1aa10af6073c9246", screen.checksum, "source rect quarters");
