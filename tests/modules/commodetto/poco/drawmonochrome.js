/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

const render = new Poco(screen);
const bitmap = new Bitmap(32, 16, Bitmap.Monochrome, new ArrayBuffer(64), 0);

render.begin();

assert.throws(SyntaxError, () => render.drawMonochrome(), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap, 0), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap, 0, 0), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap, 0, 0, 0), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, 0), "drawMonochrome requires 5 or 9 arguments");
assert.throws(SyntaxError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, 0, 0), "drawMonochrome requires 5 or 9 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.drawMonochrome(bitmap, value, value, value, value, value, value, value, value);
assert.sameValue(8, count, "drawMonochrome coerces to number");

assert.sameValue(undefined, render.drawMonochrome(bitmap, 0, 0, 0, 0), "drawMonochrome returns undefined");

assert.throws(SyntaxError, () => render.drawMonochrome(new $TESTMC.HostObject, 0, 0, 0, 0), "drawMonochrome rejects fake bitmap");
assert.throws(SyntaxError, () => render.drawMonochrome({}, 0, 0, 0, 0), "drawMonochrome rejects fake bitmap");

assert.throws(SyntaxError, () => render.drawMonochrome.call(new $TESTMC.HostObject, bitmap, 0, 0, 0, 0), "drawMonochrome with non-poco this");

render.drawMonochrome(bitmap, 0, undefined, 0, 0);
render.drawMonochrome(bitmap, undefined, 0, 0, 0);
render.drawMonochrome(bitmap, undefined, undefined, 0, 0);

assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, -1, 0, 5, 5), "source x must be non-negative");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, -1, 5, 5), "source y must be non-negative");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, 0, -1, 5), "source w must be non-negative");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, 0, 5, -1), "source h must be non-negative");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 33, 0, 5, 5), "source x must be less than width");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, 17, 5, 5), "source y must be less than height");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 8, 0, 25, 0), "source width must be less than width");
assert.throws(RangeError, () => render.drawMonochrome(bitmap, 0, 0, 0, 0, 0, 8, 5, 9), "source height must be less than height");
