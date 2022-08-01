/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

const render = new Poco(screen);
const bitmap = new Bitmap(16, 16, Bitmap.Default, new ArrayBuffer(1024), 0);
const mask = new Bitmap(8, 8, Bitmap.Gray16, new ArrayBuffer(256), 0);

render.begin();

assert.throws(SyntaxError, () => render.drawMasked(), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 0), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 0, 0), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 0, 0, mask), "drawMasked requires 9 (or or 10) arguments");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 0, 0, mask, 0), "drawMasked requires 9 (or or 10) arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.drawMasked(bitmap, value, value, value, value, value, value, mask, value, value, value);
assert.sameValue(9, count, "drawMasked coerces to number");

assert.sameValue(undefined, render.drawMasked(bitmap, 0, 0, 0, 0, 0, 0, mask, 0, 0), "drawMasked returns undefined");

assert.throws(SyntaxError, () => render.drawMasked(new $TESTMC.HostObject, 0, 0, 0, 0, 0, 0, mask, 0, 0), "drawMasked rejects fake bitmap");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 0, 0, new $TESTMC.HostObject, 0, 0), "drawMasked rejects fake bitmap");
assert.throws(SyntaxError, () => render.drawMasked({}, 0, 0, 0, 0, 0, 0, mask, 0, 0), "drawMasked rejects fake bitmap");
assert.throws(SyntaxError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 0, 0, {}, 0, 0), "drawMasked rejects fake bitmap");

assert.throws(SyntaxError, () => render.drawMasked.call(new $TESTMC.HostObject, bitmap, 0, 0, 0, 0, 0, 0, mask, 0 ,0), "drawMasked with non-poco this");

assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, -1, 0, 5, 5, mask, 0, 0), "source x must be non-negative");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, -1, 5, 5, mask, 0, 0), "source y must be non-negative");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, -1, 5, mask, 0, 0), "source w must be non-negative");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 5, -1, mask, 0, 0), "source h must be non-negative");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 17, 0, 5, 5, mask, 0, 0), "source x must be less than width");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 17, 5, 5, mask, 0, 0), "source y must be less than height");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 8, 0, 9, 0, mask, 0, 0), "source width must be less than width");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 8, 5, 9, mask, 0, 0), "source height must be less than height");

assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 16, 16, mask, 0, 0), "mask cannot be smaller than bitmap src");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 8, 8, mask, -1, 0), "mask x must be non-negative");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 8, 8, mask, 0, -1), "mask y must be non-negative");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 8, 8, mask, 8, 0), "mask x must be less than mask width");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 8, 8, mask, 0, 8), "mask y must be less than mask height");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 8, 8, mask, 4, 0), "mask cannot be width clipped");
assert.throws(RangeError, () => render.drawMasked(bitmap, 0, 0, 0, 0, 8, 8, mask, 0, 4), "mask cannot be height clipped");
