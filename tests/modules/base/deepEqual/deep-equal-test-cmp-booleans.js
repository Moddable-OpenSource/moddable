/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('booleans', function (t) {
  t.deepEqualTest(
    true,
    true,
    'trues',
    true,
    true,
    false
  );

  t.deepEqualTest(
    false,
    false,
    'falses',
    true,
    true,
    false
  );

  t.deepEqualTest(
    true,
    false,
    'true and false',
    false,
    false
  );

  t.end();
});

