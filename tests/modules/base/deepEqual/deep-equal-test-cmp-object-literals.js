/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('object literals', function (t) {
  t.deepEqualTest(
    { prototype: 2 },
    { prototype: '2' },
    'two loosely equal, strictly inequal prototype properties',
    true,
    false
  );

  t.end();
});

