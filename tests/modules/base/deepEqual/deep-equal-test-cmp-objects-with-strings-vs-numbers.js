/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('objects with strings vs numbers', function (t) {
  t.deepEqualTest(
    [{ a: 3 }, { b: 4 }],
    [{ a: '3' }, { b: '4' }],
    'objects with equivalent string/number values',
    true,
    false
  );
  t.end();
});

