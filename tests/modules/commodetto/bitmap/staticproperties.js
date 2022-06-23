/*---
description: 
flags: [module]
---*/

import Bitmap from "commodetto/Bitmap";

assert.sameValue(Bitmap.Default, 1);
assert.sameValue(Bitmap.Monochrome, 3);
assert.sameValue(Bitmap.Gray16, 4);
assert.sameValue(Bitmap.Gray256, 5);
assert.sameValue(Bitmap.RGB332, 6);
assert.sameValue(Bitmap.RGB565LE, 7);
assert.sameValue(Bitmap.RGB565BE, 8);
assert.sameValue(Bitmap.RGB24, 9);
assert.sameValue(Bitmap.RGBA32, 10);
assert.sameValue(Bitmap.CLUT16, 11);
assert.sameValue(Bitmap.ARGB4444, 12);
assert.sameValue(Bitmap.RGB444, 13);
assert.sameValue(Bitmap.BGRA32, 14);
assert.sameValue(Bitmap.JPEG, 15);
assert.sameValue(Bitmap.PNG, 16);
assert.sameValue(Bitmap.CLUT256, 17);
assert.sameValue(Bitmap.CLUT32, 18);
assert.sameValue(Bitmap.RLE, 0x80);
