/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('equal', function (t) {
  t.deepEqualTest(
    { a: [2, 3], b: [4] },
    { a: [2, 3], b: [4] },
    'two equal objects',
    true,
    true,
    false
  );

  t.deepEqualTest(
    { a: 2, b: '4' },
    { a: 2, b: 4 },
    'two loosely equal, strictly inequal objects',
    true,
    false
  );

  t.deepEqualTest(
    { a: 2, b: 4 },
    { a: 2, B: 4 },
    'two inequal objects',
    false,
    false
  );

  t.deepEqualTest(
    '-000',
    false,
    '`false` and `"-000"`',
    true,
    false
  );

  t.end();
});

