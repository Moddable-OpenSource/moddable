/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

render.begin();

assert.throws(SyntaxError, () => render.fillRectangle(), "fillRectangle requires 5 arguments");
assert.throws(SyntaxError, () => render.fillRectangle(0), "fillRectangle requires 5 arguments");
assert.throws(SyntaxError, () => render.fillRectangle(0, 0), "fillRectangle requires 5 arguments");
assert.throws(SyntaxError, () => render.fillRectangle(0, 0, 0), "fillRectangle requires 5 arguments");
assert.throws(SyntaxError, () => render.fillRectangle(0, 0, 0, 0), "fillRectangle requires 5 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.fillRectangle(value, value, value, value, value);
assert.sameValue(5, count, "fillRectangle coerces to number");

assert.sameValue(undefined, render.fillRectangle(0, 0, 0, 0, 0), "fillRectangle returns undefined");

assert.throws(SyntaxError, () => render.fillRectangle.call(new $TESTMC.HostObject, 0, 0, 0, 0, 0), "fillRectangle with non-poco this");
