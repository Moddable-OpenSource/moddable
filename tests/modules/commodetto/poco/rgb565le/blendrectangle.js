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

// unclipped
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(white, 32, 10, 10, 20, 20);
render.end();

assert.sameValue("e5386bfa56b0ab7128c75199547c2178", screen.checksum, "unclipped");

// unclipped, left misaligned
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 64, 11, 10, 20, 20);
render.end();

assert.sameValue("e9f0b0d2428c96f6ad6cba9ad0eccf30", screen.checksum, "unclipped, left misaligned");

// unclipped, odd width
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(blue, 128, 10, 10, 19, 20);
render.end();

assert.sameValue("3f4c8af2019a661b1261ae1821768e16", screen.checksum, "unclipped, odd width");

// unclipped, misaligned left and right
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 192, 11, 10, 18, 20);
render.end();

assert.sameValue("ed63bd4abc67fea040bc0e0f44588e03", screen.checksum, "unclipped, misaligned left and right");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 254, 5, 10, 20, 20);
render.end();

assert.sameValue("4f8a82ad5800a3e02436606fd095fe4b", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 33, 10, 10, 25, 20);
render.end();

assert.sameValue("3b92470d2769dd169b631e07e5cd4e81", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(blue, 65, 10, 5, 20, 20);
render.end();

assert.sameValue("7c0b3364bfd5a78038c0be2aef3b93d6", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 129, 10, 10, 20, 25);
render.end();

assert.sameValue("7ba004dc389edceeed34f70dfcd0cc99", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 193, 8, 8, 22, 22);
render.end();

assert.sameValue("f6322cb0be6bd3482c374e9d69c136fb", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(blue, 191, 30, 30, 22, 22);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 127, 10, 10, 1, 20);
render.end();

assert.sameValue("2c8eeecbb64309f6c1d04782254c9112", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 253, 10, 15, 20, 1);
render.end();

assert.sameValue("0d5c2ae6b39274826f005b04e7a6e5c6", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 127, 0, 0, render.width, render.height);
render.end();

assert.sameValue("7d74ac0339c9d927e0c55038be5544d7", screen.checksum, "clipped to 1 pixel");
