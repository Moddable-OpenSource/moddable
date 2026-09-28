/*---
description: >
  Array.prototype.concat throws a TypeError when a spreadable argument would
  make the result longer than 2**53 - 1. fx_Array_prototype_concat (xsArray.c)
  cast the argument length to txIndex, which is undefined behavior above
  2**32 - 1.
flags: [onlyStrict]
---*/

var spreadable = { length: 2 ** 53 - 1 };
spreadable[Symbol.isConcatSpreadable] = true;
assert.throws(TypeError, function () { [1].concat(spreadable); }, "1 + (2**53 - 1) elements");
