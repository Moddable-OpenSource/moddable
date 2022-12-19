/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

const render = new Poco(screen);
const bitmap = new Bitmap(16, 16, Bitmap.Default, new ArrayBuffer(1024), 0);

render.begin();

assert.throws(SyntaxError, () => render.drawBitmap(), "drawBitmap requires 3 or 7 arguments");
assert.throws(SyntaxError, () => render.drawBitmap(bitmap), "drawBitmap requires 3 or 7 arguments");
assert.throws(SyntaxError, () => render.drawBitmap(bitmap, 0), "drawBitmap requires 3 or 7 arguments");
assert.throws(SyntaxError, () => render.drawBitmap(bitmap, 0, 0, 0), "drawBitmap requires 3 or 7 arguments");
assert.throws(SyntaxError, () => render.drawBitmap(bitmap, 0, 0, 0, 0), "drawBitmap requires 3 or 7 arguments");
assert.throws(SyntaxError, () => render.drawBitmap(bitmap, 0, 0, 0, 0, 0), "drawBitmap requires 3 or 7 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.drawBitmap(bitmap, value, value, value, value, value, value);
assert.sameValue(6, count, "drawBitmap coerces to number");

assert.sameValue(undefined, render.drawBitmap(bitmap, 0, 0), "drawBitmap returns undefined");

assert.throws(SyntaxError, () => render.drawBitmap(new $TESTMC.HostObject, 0, 0), "drawBitmap rejects fake bitmap");
assert.throws(SyntaxError, () => render.drawBitmap({}, 0, 0), "drawBitmap rejects fake bitmap");

assert.throws(SyntaxError, () => render.drawBitmap.call(new $TESTMC.HostObject, bitmap, 0, 0), "drawBitmap with non-poco this");

assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, -1, 0, 5, 5), "source x must be non-negative");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 0, -1, 5, 5), "source y must be non-negative");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 0, 0, -1, 5), "source w must be non-negative");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 0, 0, 5, -1), "source h must be non-negative");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 17, 0, 5, 5), "source x must be less than width");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 0, 17, 5, 5), "source y must be less than height");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 8, 0, 9, 0), "source width must be less than width");
assert.throws(RangeError, () => render.drawBitmap(bitmap, 0, 0, 0, 8, 5, 9), "source height must be less than height");
