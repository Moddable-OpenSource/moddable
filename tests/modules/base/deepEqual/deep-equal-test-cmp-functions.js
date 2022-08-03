/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('functions', function (t) {
  function f() {}

  t.deepEqualTest(f, f, 'a function and itself', true, true, true);
  t.deepEqualTest([f], [f], 'a function and itself in an array', true, true, true);

  t.deepEqualTest(function () {}, function () {}, 'two distinct functions', false, false, true);
  t.deepEqualTest([function () {}], [function () {}], 'two distinct functions in an array', false, false, true);

  t.deepEqualTest(f, {}, 'function and object', false, false, true);
  t.deepEqualTest([f], [{}], 'function and object in an array', false, false, true);

  t.end();
});

