/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";
import Resource from "Resource";
import parseRLE from "commodetto/parseRLE";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");
assert((240 === screen.width) && (320 === screen.height), "unexpected screen");

const render = new Poco(screen);

const black = render.makeColor(0, 0, 0);
const white = render.makeColor(255, 255, 255);
const red = render.makeColor(255, 0, 0);
const green = render.makeColor(0, 255, 0);
const blue = render.makeColor(0, 0, 255);

const bitmap = parseRLE(new Resource("circleish-alpha.bm4"));		// 40 x 40

// even align
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10);
render.end();

assert.sameValue("9fcf86045c85d8afb4c56857e1cf38a8", screen.checksum, "even align");

// odd align
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 11, 10);
render.end();

assert.sameValue("f1ab6862942d3c1ce77b653f886d1186", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 5, 10);
render.end();

assert.sameValue("7d43c540bb6bc4b640e9af3428e497c4", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, 15, 10);
render.end();

assert.sameValue("1553755c7296adb86c2868a9ecd04762", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 5);
render.end();

assert.sameValue("e3d17a5ba1fb44059a948c8168cc95d3", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 15);
render.end();

assert.sameValue("2ad7d247e77b27d7c8dfc6c95ca7cb65", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10, 10);
render.end();

assert.sameValue("f353f8592ed926e897af54bfc3000103", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, 50, 50);
render.end();

assert.sameValue("1930253219d72b0b330ac69024017f3f", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10 - 20, 10);
render.end();

assert.sameValue("deb3c9c87ed8d0c2e165146c8dcf210d", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 40, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 10 - 20);
render.end();

assert.sameValue("eba6f6e4691389ffff3ecb73808a7217", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10 - 20, 10 - 20);
render.end();

assert.sameValue("faf82d36a6a38b5f4e34f3bb8f5136e5", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 50, 50);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10, 0, 0, 20, 20);
	render.drawGray(bitmap, red, 31, 10, 20, 0, 20, 20);
	render.drawGray(bitmap, green, 10, 29, 0, 20, 20, 20);
	render.drawGray(bitmap, blue, 31, 29, 20, 20, 20, 20);
render.end();

assert.sameValue("aca2fc2ed3aeebdd241dcff514362077", screen.checksum, "source rect quarters");
