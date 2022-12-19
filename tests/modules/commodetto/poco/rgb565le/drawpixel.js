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

// even align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawPixel(white, 10, 10);
render.end();

assert.sameValue("d0e6aedd200c26197fda655326a16040", screen.checksum, "even align");

// odd align
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawPixel(red, 11, 10);
render.end();

assert.sameValue("9d8424e46862ef1067a3cdf3b5d9b78f", screen.checksum, "odd align");

// clipped out
render.begin(10, 10, 20, 20);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawPixel(green, 0, 0);
render.end();

assert.sameValue("cf69901e6d4609009dff8be5b3045c96", screen.checksum, "clipped out");

