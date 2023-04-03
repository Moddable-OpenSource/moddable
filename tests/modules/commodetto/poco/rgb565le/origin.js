/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");
assert((240 === screen.width) && (320 === screen.height), "unexpected screen");

const render = new Poco(screen);

let black = render.makeColor(0, 0, 0);
let blue = render.makeColor(0, 0, 255);
let green = render.makeColor(0, 255, 0);
let red = render.makeColor(255, 0, 0);
let gray = render.makeColor(128, 128, 128);

render.begin();
	render.fillRectangle(gray, 0, 0, render.width, render.height);

	render.origin(20, 20);
	render.fillRectangle(red, 0, 0, 80, 40);

	render.origin(65, 65);
	render.fillRectangle(green, 0, 0, 80, 40);

	render.origin(65, 65);
	render.fillRectangle(blue, 0, 0, 80, 40);

	render.blendRectangle(black, 128, -10, -10, 30, 20);
	render.origin();

	render.blendRectangle(black, 128, -10, -10, 30, 20);
	render.origin();

	render.blendRectangle(black, 128, -10, -10, 30, 20);
	render.origin();
render.end();

assert.sameValue("5fffd9c5f5fd284e1a8375dfac42ee17", screen.checksum, "image mismatch");
