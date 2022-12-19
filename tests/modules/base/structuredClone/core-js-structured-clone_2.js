/*---
description: adapted from https://github.com/zloirock/core-js/blob/master/tests/tests/web.structured-clone.js
flags: [module]
includes: [compareArray.js]
---*/

import { structuredClone, GLOBAL, NODE, from, assign, getPrototypeOf, keys, fromSource, QUnit, cloneTest, cloneObjectTest } from "./core-js-structured-clone_FIXTURE.js"

// Regular Expressions
QUnit.test('RegExp', assert => {
  const regexes = [
    new RegExp(),
    /abc/,
    /abc/g,
    /abc/i,
    /abc/gi,
    /abc/,
    /abc/g,
    /abc/i,
    /abc/gi,
  ];

  const giuy = fromSource('/abc/giuy');
  if (giuy) regexes.push(giuy);

  for (const regex of regexes) cloneObjectTest(assert, regex, (orig, clone) => {
    assert.same(orig.toString(), clone.toString(), `regex ${ regex }`);
  });
});

if (fromSource('ArrayBuffer.prototype.slice || DataView')) {
  // ArrayBuffer
  if (typeof Uint8Array == 'function') QUnit.test('ArrayBuffer', assert => { // Crashes
    cloneObjectTest(assert, new Uint8Array([0, 1, 254, 255]).buffer, (orig, clone) => {
      assert.arrayEqual(new Uint8Array(orig), new Uint8Array(clone));
    });
  });

  // TODO SharedArrayBuffer

  // Array Buffer Views
  if (typeof Int8Array != 'undefined') {
    QUnit.test('%TypedArray%', assert => {
      const arrays = [
        new Uint8Array([]),
        new Uint8Array([0, 1, 254, 255]),
        new Uint16Array([0x0000, 0x0001, 0xFFFE, 0xFFFF]),
        new Uint32Array([0x00000000, 0x00000001, 0xFFFFFFFE, 0xFFFFFFFF]),
        new Int8Array([0, 1, 254, 255]),
        new Int16Array([0x0000, 0x0001, 0xFFFE, 0xFFFF]),
        new Int32Array([0x00000000, 0x00000001, 0xFFFFFFFE, 0xFFFFFFFF]),
        new Float32Array([-Infinity, -1.5, -1, -0.5, 0, 0.5, 1, 1.5, Infinity, NaN]),
        new Float64Array([-Infinity, -Number.MAX_VALUE, -Number.MIN_VALUE, 0, Number.MIN_VALUE, Number.MAX_VALUE, Infinity, NaN]),
      ];

      if (typeof Uint8ClampedArray != 'undefined') {
        arrays.push(new Uint8ClampedArray([0, 1, 254, 255]));
      }

      for (const array of arrays) cloneObjectTest(assert, array, (orig, clone) => {
        assert.arrayEqual(orig, clone);
      });
    });

    if (typeof DataView != 'undefined') QUnit.test('DataView', assert => {
      const array = new Int8Array([1, 2, 3, 4]);
      const view = new DataView(array.buffer);

      cloneObjectTest(assert, array, (orig, clone) => {
        assert.same(orig.byteLength, clone.byteLength);
        assert.same(orig.byteOffset, clone.byteOffset);
        assert.arrayEqual(new Int8Array(view.buffer), array);
      });
    });
  }
}

