/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('error = Object', function (t) {
  t.deepEqualTest(
    new Error('a'),
    { message: 'a' },
    'error = Object',
    false,
    false
  );

  t.end();
});

