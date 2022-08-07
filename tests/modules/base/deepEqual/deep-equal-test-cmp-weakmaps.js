/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('WeakMaps', { skip: true }, function (t) {
  t.deepEqualTest(
    new WeakMap([[Object, null], [Function, true]]),
    new WeakMap([[Function, true], [Object, null]]),
    'two equal WeakMaps',
    true,
    true
  );

  t.deepEqualTest(
    new WeakMap([[Object, null]]),
    new WeakMap([[Object, true]]),
    'two WeakMaps with inequal values on the same key',
    true,
    true
  );

  t.deepEqualTest(
    new WeakMap([[Object, null], [Function, true]]),
    new WeakMap([[Object, null]]),
    'two inequal WeakMaps',
    true,
    true
  );

  t.end();
});

