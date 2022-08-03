/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

var isBrokenNode = isNode && process.env.ASSERT && semver.satisfies(process.version, '<= 13.3.0');

test('fake arrays: extra keys will be tested', { skip: !hasDunderProto || isBrokenNode }, function (t) {
  var a = tag({
    __proto__: Array.prototype,
    0: 1,
    1: 1,
    2: 'broken',
    length: 2
  }, 'Array');
  if (Object.defineProperty) {
    Object.defineProperty(a, 'length', {
      enumerable: false
    });
  }

  t.deepEqualTest(a, [1, 1], 'fake and real array with same contents and [[Prototype]]', false, false);

  var b = tag(/abc/, 'Array');
  b.length = 3;
  b.__proto__ = Array.prototype; // eslint-disable-line no-proto
  if (Object.defineProperty) {
    Object.defineProperty(b, 'length', {
      enumerable: false
    });
  }
  t.deepEqualTest(b, ['a', 'b', 'c'], 'regex faking as array, and array', false, false);

  t.end();
});

