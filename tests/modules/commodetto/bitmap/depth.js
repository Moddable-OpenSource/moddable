/*---
description: 
flags: [module]
---*/

import Bitmap from "commodetto/Bitmap";

assert.sameValue(1, Bitmap.depth(Bitmap.Monochrome));
assert.sameValue(4, Bitmap.depth(Bitmap.Gray16));
assert.sameValue(8, Bitmap.depth(Bitmap.Gray256));
assert.sameValue(8, Bitmap.depth(Bitmap.RGB332));
assert.sameValue(16, Bitmap.depth(Bitmap.RGB565LE));
assert.sameValue(16, Bitmap.depth(Bitmap.RGB565BE));
assert.sameValue(24, Bitmap.depth(Bitmap.RGB24));
assert.sameValue(32, Bitmap.depth(Bitmap.RGBA32));
assert.sameValue(4, Bitmap.depth(Bitmap.CLUT16));
assert.sameValue(16, Bitmap.depth(Bitmap.ARGB4444));
assert.sameValue(12, Bitmap.depth(Bitmap.RGB444));
assert.sameValue(8, Bitmap.depth(Bitmap.CLUT256));
assert.sameValue(5, Bitmap.depth(Bitmap.CLUT32));

assert.throws(Error, () => Bitmap.depth(Bitmap.JPEG), "JPEG has no depth");
assert.throws(Error, () => Bitmap.depth(Bitmap.PNG), "PNG has no depth");
