/*---
description: adapted from https://github.com/zloirock/core-js/blob/master/tests/tests/web.structured-clone.js
flags: [module]
includes: [compareArray.js]
---*/

import { structuredClone, GLOBAL, NODE, from, assign, getPrototypeOf, keys, fromSource, QUnit, cloneTest, cloneObjectTest } from "./core-js-structured-clone_FIXTURE.js"

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


