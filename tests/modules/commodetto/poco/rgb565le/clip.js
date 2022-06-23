/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");

const render = new Poco(screen);

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);
let green = render.makeColor(0, 255, 0);
let red = render.makeColor(255, 0, 0);

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);

	render.clip(20, 20, render.width - 40, render.height - 40);
	render.fillRectangle(white, 0, 0, render.width, render.height);

	render.clip(40, 40, render.width >> 1, render.height - 80);
	render.fillRectangle(green, 0, 0, render.width, render.height);
	render.clip();

	render.clip(render.width >> 1, 40, (render.width >> 1) - 40, render.height - 80);
	render.fillRectangle(red, 0, 0, render.width, render.height);
	render.clip();

	render.clip();
render.end();

assert.sameValue("3f7bcfc81b231d62f787bea509c8646a", screen.checksum, "image mismatch");
