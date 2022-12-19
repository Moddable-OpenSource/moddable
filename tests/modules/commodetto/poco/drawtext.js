/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";

const render = new Poco(screen);
const font = parseBMF(new Resource("OpenSans-Regular-16.bf4"));

render.begin();

assert.throws(SyntaxError, () => render.drawText(), "drawText requires 5 arguments");
assert.throws(SyntaxError, () => render.drawText("a"), "drawText requires 5 arguments");
assert.throws(SyntaxError, () => render.drawText("a", font), "drawText requires 5 arguments");
assert.throws(SyntaxError, () => render.drawText("a", font, 0), "drawText requires 5 arguments");
assert.throws(SyntaxError, () => render.drawText("a", font, 0, 0), "drawText requires 5 arguments");

render.drawText("a", font, 0, 0, 0);

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.drawText(value, font, value, value, value, value);
assert.sameValue(5, count, "drawText coerces");

assert.throws(SyntaxError, () => render.drawText("a", "12", 0, 0, 0), "invalid font");
assert.throws(SyntaxError, () => render.drawText("a", new ArrayBuffer, 0, 0, 0), "invalid font");
assert.throws(SyntaxError, () => render.drawText("a", Uint8Array.of(32), 0, 0, 0), "invalid font");

assert.sameValue(undefined, render.drawText("a", font, 0, 0, 0), "drawText returns undefined");

assert.throws(SyntaxError, () => render.drawText.call(new $TESTMC.HostObject, "a", font, 0, 0, 0), "drawText with non-poco this");
