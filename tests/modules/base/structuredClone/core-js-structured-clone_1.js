/*---
description: adapted from https://github.com/zloirock/core-js/blob/master/tests/tests/web.structured-clone.js
flags: [module]
includes: [compareArray.js,deepEqual.js]
---*/

import { GLOBAL, NODE, from, assign, getPrototypeOf, keys, fromSource, QUnit, cloneTest, cloneObjectTest } from "./core-js-structured-clone_FIXTURE.js"

QUnit.test('identity', assert => {
  assert.isFunction(structuredClone, 'structuredClone is a function');
  assert.name(structuredClone, 'structuredClone');
  assert.arity(structuredClone, 1);
  if (!NODE) assert.looksNative(structuredClone);
  assert.throws(() => structuredClone(), 'throws without arguments');
  assert.same(structuredClone(1, null), 1, 'null as options');
  assert.same(structuredClone(1, undefined), 1, 'undefined as options');
});

// ECMAScript types

// Primitive values: Undefined, Null, Boolean, Number, BigInt, String
const booleans = [false, true];
const numbers = [
  NaN,
  -Infinity,
  -Number.MAX_VALUE,
  -0xFFFFFFFF,
  -0x80000000,
  -0x7FFFFFFF,
  -1,
  -Number.MIN_VALUE,
  -0,
  0,
  1,
  Number.MIN_VALUE,
  0x7FFFFFFF,
  0x80000000,
  0xFFFFFFFF,
  Number.MAX_VALUE,
  Infinity,
];

const bigints = fromSource(`[
  -12345678901234567890n,
  -1n,
  0n,
  1n,
  12345678901234567890n,
]`) || [];

const strings = [
  '',
  'this is a sample string',
  'null(\0)',
];

QUnit.test('primitives', assert => {
  const primitives = [undefined, null].concat(booleans, numbers, bigints, strings);

  for (const value of primitives) cloneTest(value, (orig, clone) => {
    assert.same(orig, clone, 'primitives should be same after cloned');
  });
});

// "Primitive" Objects (Boolean, Number, BigInt, String)
QUnit.test('primitive objects', assert => {
  const primitives = [].concat(booleans, numbers, bigints, strings);

  for (const value of primitives) cloneObjectTest(assert, Object(value), (orig, clone) => {
    assert.same(orig.valueOf(), clone.valueOf(), 'primitive wrappers should have same value');
  });
});

// Dates
QUnit.test('Date', assert => {
  const dates = [
    new Date(-1e13),
    new Date(-1e12),
    new Date(-1e9),
    new Date(-1e6),
    new Date(-1e3),
    new Date(0),
    new Date(1e3),
    new Date(1e6),
    new Date(1e9),
    new Date(1e12),
    new Date(1e13),
  ];

  for (const date of dates) cloneTest(date, (orig, clone) => {
    assert.notSame(orig, clone);
    assert.same(typeof clone, 'object');
    assert.same(getPrototypeOf(orig), getPrototypeOf(clone));
    assert.same(orig.valueOf(), clone.valueOf());
  });
});

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

