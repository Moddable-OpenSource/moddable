/*---
description: adapted from https://github.com/zloirock/core-js/blob/master/tests/tests/web.structured-clone.js
flags: [module]
includes: [compareArray.js]
---*/

import { structuredClone, GLOBAL, NODE, from, assign, getPrototypeOf, keys, fromSource, QUnit, cloneTest, cloneObjectTest } from "./core-js-structured-clone_FIXTURE.js"

// Arrays
QUnit.test('Array', assert => {
  const arrays = [
    [],
    [1, 2, 3],
    assign(
      ['foo', 'bar'],
      { 10: true, 11: false, 20: 123, 21: 456, 30: null }),
    assign(
      ['foo', 'bar'],
      { a: true, b: false, foo: 123, bar: 456, '': null }),
  ];

  for (const array of arrays) cloneObjectTest(assert, array, (orig, clone) => {
    assert.deepEqual(orig, clone, `array content should be same: ${ array }`);
    assert.deepEqual(keys(orig), keys(clone), `array key should be same: ${ array }`);
    for (const key of keys(orig)) {
      assert.same(orig[key], clone[key], `Property ${ key }`);
    }
  });
});

// Objects
QUnit.test('Object', assert => {
  cloneObjectTest(assert, { foo: true, bar: false }, (orig, clone) => {
    assert.deepEqual(keys(orig), keys(clone));
    for (const key of keys(orig)) {
      assert.same(orig[key], clone[key], `Property ${ key }`);
    }
  });
});

// Non-serializable types
QUnit.test('Non-serializable types', assert => {
  const nons = [
    function () { return 1; },
    Symbol('desc'),
    GLOBAL,
  ];

  const event = fromSource('new Event("")');
  const port = fromSource('new MessageChannel().port1');

  // NodeJS events are simple objects
  if (event && !NODE) nons.push(event);
  if (port) nons.push(port);

  for (const it of nons) {
    // native NodeJS `structuredClone` throws a `TypeError` on transferable non-serializable instead of `DOMException`
    // https://github.com/nodejs/node/issues/40841
    assert.throws(() => structuredClone(it));
  }
});
