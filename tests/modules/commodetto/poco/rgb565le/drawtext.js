/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";

assert.sameValue(Bitmap.RGB565LE, screen.pixelFormat, "requires RGB565LE output");

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

assert.sameValue("53934218d25b83bba571687c638bbcaa", screen.checksum, "simple white");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Text test", font, blue, 10, 10);
render.end();

assert.sameValue("94b2e106da67df3e04092163576b0772", screen.checksum, "simple blue");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, blue, 10, 10);
render.end();

assert.sameValue("88c42eee07f4042a67a68720b84a32c3", screen.checksum, "simple blue swapped");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText(12345.6, font, blue, 10, 10);
render.end();

assert.sameValue("9eeacdb5b5f9f3ceba79234b30c2bda3", screen.checksum, "number coersion");

let width = render.getTextWidth("Test text", font);
render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, white, 10 - (width / 2), 10);
render.end();

assert.sameValue("ab1b4e76f073d6b9b07f1ecafed12785", screen.checksum, "clip left");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, white, render.width - (width / 2), 10);
render.end();

assert.sameValue("3c6af7361eb613371f48157ebe9b1f57", screen.checksum, "clip right");

render.begin(0, 0, render.width, 40);
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.drawText("Test text", font, white, render.width - (width / 2), 10, width / 2);
render.end();

assert.sameValue("3ac60486adf6ad242c0e29e53b7ce1e1", screen.checksum, "clip right elipsis");
