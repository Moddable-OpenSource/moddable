/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

const render = new Poco(screen);
const bitmap = new Bitmap(16, 16, Bitmap.Default, new ArrayBuffer(1024), 0);

render.begin();

assert.throws(SyntaxError, () => render.fillPattern(), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap, 0), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap, 0, 0), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap, 0, 0, 0), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap, 0, 0, 0, 0, 0), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap, 0, 0, 0, 0, 0, 0), "fillPattern requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.fillPattern(bitmap, 0, 0, 0, 0, 0, 0, 0), "fillPattern requires 5 or 9 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.fillPattern(bitmap, value, value, value, value, value, value, value, value);
assert.sameValue(8, count, "fillPattern coerces to number");

assert.sameValue(undefined, render.fillPattern(bitmap, 0, 0, 0, 0), "fillPattern returns undefined");

assert.throws(SyntaxError, () => render.fillPattern(new $TESTMC.HostObject, 0, 0, 0, 0), "fillPattern rejects fake bitmap");
assert.throws(SyntaxError, () => render.fillPattern({}, 0, 0, 0, 0), "fillPattern rejects fake bitmap");

assert.throws(SyntaxError, () => render.fillPattern.call(new $TESTMC.HostObject, bitmap, 0, 0, 0, 0), "fillPattern with non-poco this");

assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, -1, 0, 5, 5), "source x must be non-negative");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 0, -1, 5, 5), "source y must be non-negative");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 0, 0, -1, 5), "source w must be non-negative");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 0, 0, 5, -1), "source h must be non-negative");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 17, 0, 5, 5), "source x must be less than width");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 0, 17, 5, 5), "source y must be less than height");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 8, 0, 9, 0), "source width must be less than width");
assert.throws(RangeError, () => render.fillPattern(bitmap, 0, 0, 50, 50, 0, 8, 5, 9), "source height must be less than height");
