/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('arrays and objects', function (t) {
  t.deepEqualTest([], {}, 'empty array and empty object', false, false);
  t.deepEqualTest([], { length: 0 }, 'empty array and empty arraylike object', false, false);
  t.deepEqualTest([1], { 0: 1 }, 'array and similar object', false, false);

  t.end();
});

