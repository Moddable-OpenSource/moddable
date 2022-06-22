/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

render.begin();

assert.throws(SyntaxError, () => render.drawPixel(), "drawPixel requires 3 arguments");
assert.throws(SyntaxError, () => render.drawPixel(0), "drawPixel requires 3 arguments");
assert.throws(SyntaxError, () => render.drawPixel(0, 0), "drawPixel requires 3 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.drawPixel(value, value, value);
assert.sameValue(3, count, "drawPixel coerces to number");

assert.sameValue(undefined, render.drawPixel(0, 0, 0), "drawPixel returns undefined");

assert.throws(SyntaxError, () => render.drawPixel.call(new $TESTMC.HostObject, 0, 0, 0, 0, 0, 0), "drawPixel with non-poco this");
