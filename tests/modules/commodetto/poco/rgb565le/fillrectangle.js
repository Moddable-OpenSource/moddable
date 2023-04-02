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
	render.fillRectangle(white, 10, 10, 20, 20);
render.end();

assert.sameValue("6c4d33b2730c35eae4d2fad25159f7d8", screen.checksum, "unclipped");

// unclipped, left misaligned
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 11, 10, 20, 20);
render.end();

assert.sameValue("32367f3cf1485af6f7588047fb9bf577", screen.checksum, "unclipped, left misaligned");

// unclipped, odd width
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(blue, 10, 10, 19, 20);
render.end();

assert.sameValue("4e35879eada66b3e905fc2d3e8b83eaf", screen.checksum, "unclipped, odd width");

// unclipped, misaligned left and right
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 11, 10, 18, 20);
render.end();

assert.sameValue("be6adaea3a45c37171cc85011c16540f", screen.checksum, "unclipped, misaligned left and right");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 5, 10, 20, 20);
render.end();

assert.sameValue("c0a4081125010ce6e7542bcfcebd4eb8", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 10, 10, 25, 20);
render.end();

assert.sameValue("8c133f250201106034e7027c1dfbbfdb", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(blue, 10, 5, 20, 20);
render.end();

assert.sameValue("dd97a9ab83492869867acec5817f4cb3", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 10, 10, 20, 25);
render.end();

assert.sameValue("9c056dae1804340ab6b3b11328638655", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 8, 8, 22, 22);
render.end();

assert.sameValue("8c133f250201106034e7027c1dfbbfdb", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(blue, 30, 30, 22, 22);
render.end();

assert.sameValue("44aed7e3f1ad596d9aed457079608ed9", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 10, 10, 1, 20);
render.end();

assert.sameValue("571f6329f364b2248124b60ec48d8942", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 10, 15, 20, 1);
render.end();

assert.sameValue("27f1108c34b5c8d0f3e4d3d1dc15b556", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 0, 0, render.width, render.height);
render.end();

assert.sameValue("faf82d36a6a38b5f4e34f3bb8f5136e5", screen.checksum, "clipped to 1 pixel");
