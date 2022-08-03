/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('booleans and arrays', function (t) {
  t.deepEqualTest(
    true,
    [],
    'true and an empty array',
    false,
    false
  );
  t.deepEqualTest(
    false,
    [],
    'false and an empty array',
    false,
    false
  );
  t.end();
});

