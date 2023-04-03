/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");
assert((240 === screen.width) && (320 === screen.height), "unexpected screen");

const render = new Poco(screen);
const font = parseBMF(new Resource("OpenSans-Regular-16.bf4"));

const black = render.makeColor(0, 0, 0);
const white = render.makeColor(255, 255, 255);
const red = render.makeColor(255, 0, 0);
const green = render.makeColor(0, 255, 0);
const blue = render.makeColor(0, 0, 255);

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Text test", font, white, 10, 10);
render.end();

assert.sameValue("2acda3821ccb15163db8823980432ffb", screen.checksum, "simple white");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Text test", font, blue, 10, 10);
render.end();

assert.sameValue("c84f6f98042369c9ea23d23802a23928", screen.checksum, "simple blue");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, blue, 10, 10);
render.end();

assert.sameValue("ea82fc86f105748075473be87b1780a1", screen.checksum, "simple blue swapped");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText(12345.6, font, blue, 10, 10);
render.end();

assert.sameValue("001c0224e3e96ec5caa5813603f3266c", screen.checksum, "number coersion");

let width = render.getTextWidth("Test text", font);
render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, white, 10 - (width / 2), 10);
render.end();

assert.sameValue("63816861cbfde1976663e2c48445f409", screen.checksum, "clip left");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, white, render.width - (width / 2), 10);
render.end();

assert.sameValue("0572496c475890ddc5798f988968b4d5", screen.checksum, "clip right");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, white, render.width - (width / 2), 10, width / 2);
render.end();

assert.sameValue("5eafce637d5e9755845fff33d20cd873", screen.checksum, "clip right elipsis");
