/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

// node 14 changed `deepEqual` to make two NaNs loosely equal. TODO, semver-major: change deep-equal in the same way.
var isNode14 = true; //isNode && process.env.ASSERT && semver.satisfies(process.version, '>= 14');

test('NaNs', function (t) {
  t.deepEqualTest(
    NaN,
    NaN,
    'two NaNs',
    isNode14,
    true
  );

  t.deepEqualTest(
    { a: NaN },
    { a: NaN },
    'two equiv objects with a NaN value',
    isNode14,
    true
  );

  t.deepEqualTest(NaN, 1, 'NaN and 1', false, false);

  t.end();
});

