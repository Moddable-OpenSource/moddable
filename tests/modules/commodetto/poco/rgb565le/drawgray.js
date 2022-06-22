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

const pixels = new Uint8Array(new ArrayBuffer(16 * 16 / 2 + 4), 4);
const bitmap = new Bitmap(16, 16, Bitmap.Gray16, pixels.buffer, 4);
pixels.fill(0x00, 0, 16 * 2);
pixels.fill(0x22, 2 * 16, 4 * 16);
pixels.fill(0x44, 4 * 16, 6 * 16);
pixels.fill(0x66, 6 * 16, 8 * 16);

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10);
render.end();

assert.sameValue("3c8e94ad9af3c7027b337a5b354dad2f", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 11, 10);
render.end();

assert.sameValue("d633ac3a925bcf009c96674294f161f1", screen.checksum, "odd align");

// clipped left
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 9, 10);
render.end();

assert.sameValue("9dc3d7fe30f49dfd612a7a3990966b54", screen.checksum, "clipped left");

// clipped right
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, 15, 10);
render.end();

assert.sameValue("c674c7cb6c57d68b7d9ce22214f82bca", screen.checksum, "clipped right");

// clipped top
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 9);
render.end();

assert.sameValue("d96f3385f4fe6ab8af4bd3c210c88001", screen.checksum, "clipped top");

// clipped bottom
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 15);
render.end();

assert.sameValue("9407ac03713a16c95f149b30ad1ebc86", screen.checksum, "clipped bottom");

// clipped all edges
render.begin(11, 11, 8, 8);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10, 10);
render.end();

assert.sameValue("c54dfad8acbbe90c75cd78f1a2239602", screen.checksum, "clipped all edges");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, blue, -10, -10);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

// 1 pixel wide
render.begin(10, 10, 1, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10);
render.end();

assert.sameValue("bb51e01881e84ec192faabe29c3139c8", screen.checksum, "1 pixel wide");

// 1 pixel high
render.begin(10, 10, 20, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, red, 10, 10);
render.end();

assert.sameValue("9e2e5e9a23f3b8f5a6e51ab1fcbed21c", screen.checksum, "1 pixel high");

// clipped to 1 pixel
render.begin(10, 10, 1, 1);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, green, 10, 10);
render.end();

assert.sameValue("ff1d5cb367c5cfdc7e90b42718d80bb0", screen.checksum, "clipped to 1 pixel");

// source rect quarters
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawGray(bitmap, white, 10, 10, 0, 0, 8, 8);
	render.drawGray(bitmap, red, 19, 10, 8, 0, 8, 8);
	render.drawGray(bitmap, green, 10, 19, 0, 8, 8, 8);
	render.drawGray(bitmap, blue, 19, 19, 8, 8, 8, 8);
render.end();

assert.sameValue("affe01590ae6c4bacea329c16e1ebfd8", screen.checksum, "source rect quarters");
