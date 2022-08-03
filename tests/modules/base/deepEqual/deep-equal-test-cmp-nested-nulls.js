/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('nested nulls', function (t) {
  t.deepEqualTest(
    [null, null, null],
    [null, null, null],
    'same-length arrays of nulls',
    true,
    true,
    true
  );
  t.end();
});

