/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/892
negative:
  type: RangeError
---*/

function main() {
var a4 = [1111111111,1111111111,1111111111,1111111111,1111111111];
var a5 = [11111111111111111111];
var a8 = ``;
var a9 = 0;
var a10 = Uint32Array;
var a11 = new Uint8ClampedArray();
({"buffer":a9,"byteLength":a10,"byteOffset":a8,} = a11);
var a13 = new Uint8ClampedArray(a9,1111111111,...a4);
var a14 = new Uint32Array(Symbol,111111111111111,...a5,...a13);
}
main();
