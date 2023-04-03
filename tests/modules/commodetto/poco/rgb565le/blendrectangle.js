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

// unclipped
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(white, 32, 10, 10, 20, 20);
render.end();

assert.sameValue("d7d221e31acf938ebdcc403fa9f322d5", screen.checksum, "unclipped");

// unclipped, left misaligned
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 64, 11, 10, 20, 20);
render.end();

assert.sameValue("9c35f54cf69e7ae60e53096ea549f70f", screen.checksum, "unclipped, left misaligned");

// unclipped, odd width
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(blue, 128, 10, 10, 19, 20);
render.end();

assert.sameValue("a6a1b6af9a22717bbe620742260ec012", screen.checksum, "unclipped, odd width");

// unclipped, misaligned left and right
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 192, 11, 10, 18, 20);
render.end();

assert.sameValue("45a9bd2e7746ae1d1e337af42da46f25", screen.checksum, "unclipped, misaligned left and right");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 254, 5, 10, 20, 20);
render.end();

assert.sameValue("c0a4081125010ce6e7542bcfcebd4eb8", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 33, 10, 10, 25, 20);
render.end();

assert.sameValue("770a97eed1e6e2ca511b1a1d6c76f6bd", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(blue, 65, 10, 5, 20, 20);
render.end();

assert.sameValue("70a4783e4ef7176d6ac52e636851f8e6", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 129, 10, 10, 20, 25);
render.end();

assert.sameValue("39560158a3f4792802423c332ded59fa", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 193, 8, 8, 22, 22);
render.end();

assert.sameValue("05043758124df12e77df6928e46be5a7", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(blue, 191, 30, 30, 22, 22);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 127, 10, 10, 1, 20);
render.end();

assert.sameValue("e76c6e625d77b5f0ad90d94a4efa0372", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(red, 253, 10, 15, 20, 1);
render.end();

assert.sameValue("27f1108c34b5c8d0f3e4d3d1dc15b556", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(green, 127, 0, 0, render.width, render.height);
render.end();

assert.sameValue("168ac8bca62a2cbdd5b93f5165f13313", screen.checksum, "clipped to 1 pixel");
