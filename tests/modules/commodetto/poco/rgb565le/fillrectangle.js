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
	render.fillRectangle(white, 10, 10, 20, 20);
render.end();

assert.sameValue("510e3be62379bd1473825793847dbbba", screen.checksum, "unclipped");

// unclipped, left misaligned
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 11, 10, 20, 20);
render.end();

assert.sameValue("6d7fdd08f37cdd18e5818f07f7e3b0a4", screen.checksum, "unclipped, left misaligned");

// unclipped, odd width
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(blue, 10, 10, 19, 20);
render.end();

assert.sameValue("b2af6572c9d960879f1d657575890400", screen.checksum, "unclipped, odd width");

// unclipped, misaligned left and right
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 11, 10, 18, 20);
render.end();

assert.sameValue("5962874eba3fdf36e3ebc1a56fe8a6e9", screen.checksum, "unclipped, misaligned left and right");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 5, 10, 20, 20);
render.end();

assert.sameValue("4f8a82ad5800a3e02436606fd095fe4b", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 10, 10, 25, 20);
render.end();

assert.sameValue("e4140e87e5863e01e5ac923bca99caba", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(blue, 10, 5, 20, 20);
render.end();

assert.sameValue("f52745ae9db5dfd79aacaff9af16242c", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 10, 10, 20, 25);
render.end();

assert.sameValue("19627e0bbe539bfaeaf29645925a921b", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 8, 8, 22, 22);
render.end();

assert.sameValue("e4140e87e5863e01e5ac923bca99caba", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(blue, 30, 30, 22, 22);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 10, 10, 1, 20);
render.end();

assert.sameValue("83d7395bc6060984ff70125f79e4cea7", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(red, 10, 15, 20, 1);
render.end();

assert.sameValue("0d5c2ae6b39274826f005b04e7a6e5c6", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.fillRectangle(green, 0, 0, render.width, render.height);
render.end();

assert.sameValue("ff1d5cb367c5cfdc7e90b42718d80bb0", screen.checksum, "clipped to 1 pixel");
