/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";
import Resource from "Resource";
import parseRLE from "commodetto/parseRLE";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");

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

assert.sameValue("0de3fd7c8ef07c4c7c134862bb7f4eb2", screen.checksum, "even align");

// odd align
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 11, 10);
render.end();

assert.sameValue("374f2ebaf82f3d107a29bfc12b2a4dc3", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 5, 10);
render.end();

assert.sameValue("bc7778731565159b70fd754feb019f39", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, 15, 10);
render.end();

assert.sameValue("9e5f483bd3e8a916d4ad9bcd335dd22c", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 5);
render.end();

assert.sameValue("b8fb2fb5549889c8f35a872099b44eab", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 15);
render.end();

assert.sameValue("c786f5058bf5a65e1f6fa9a3a653ec1c", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10, 10);
render.end();

assert.sameValue("051f514809f492b98791303e1bff1eff", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 40, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, 50, 50);
render.end();

assert.sameValue("30521827c7bbe1bcae03df0c266d2844", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10 - 20, 10);
render.end();

assert.sameValue("a9817283f1e36bc520f20b56c57ce17a", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 40, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 10 - 20);
render.end();

assert.sameValue("f4be53c0d7ed757002d978c1e6994f64", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10 - 20, 10 - 20);
render.end();

assert.sameValue("ff1d5cb367c5cfdc7e90b42718d80bb0", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 50, 50);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10, 0, 0, 20, 20);
	render.drawGray(bitmap, red, 31, 10, 20, 0, 20, 20);
	render.drawGray(bitmap, green, 10, 29, 0, 20, 20, 20);
	render.drawGray(bitmap, blue, 31, 29, 20, 20, 20, 20);
render.end();

assert.sameValue("268d8a3b2b0ef03eb87a764f69076ec6", screen.checksum, "source rect quarters");
