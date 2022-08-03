/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('zeroes', function (t) {
  t.deepEqualTest(0, -0, '0 and -0', true, false);

  t.deepEqualTest({ a: 0 }, { a: -0 }, 'two objects with a same-keyed 0/-0 value', true, false);

  t.end();
});

