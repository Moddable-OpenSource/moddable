/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

render.begin();

assert.throws(SyntaxError, () => render.blendRectangle(), "blendRectangle requires 6 arguments");
assert.throws(SyntaxError, () => render.blendRectangle(0), "blendRectangle requires 6 arguments");
assert.throws(SyntaxError, () => render.blendRectangle(0, 0), "blendRectangle requires 6 arguments");
assert.throws(SyntaxError, () => render.blendRectangle(0, 0, 0), "blendRectangle requires 6 arguments");
assert.throws(SyntaxError, () => render.blendRectangle(0, 0, 0, 0), "blendRectangle requires 6 arguments");
assert.throws(SyntaxError, () => render.blendRectangle(0, 0, 0, 0, 0), "blendRectangle requires 6 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.blendRectangle(value, value, value, value, value, value);
assert.sameValue(6, count, "blendRectangle coerces to number");

assert.sameValue(undefined, render.blendRectangle(0, 0, 0, 0, 0, 0), "blendRectangle returns undefined");

assert.throws(SyntaxError, () => render.blendRectangle.call(new $TESTMC.HostObject, 0, 0, 0, 0, 0, 0), "blendRectangle with non-poco this");
