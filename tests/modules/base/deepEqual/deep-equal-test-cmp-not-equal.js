/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('not equal', function (t) {
  t.deepEqualTest(
    { x: 5, y: [6] },
    { x: 5, y: 6 },
    'two inequal objects are',
    false,
    false
  );

  t.end();
});

