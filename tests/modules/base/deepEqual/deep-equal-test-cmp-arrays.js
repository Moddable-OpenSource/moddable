/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Arrays', function (t) {
  var a = [];
  var b = [];
  b.foo = true;

  t.deepEqualTest(
    a,
    b,
    'two identical arrays, one with an extra property',
    false,
    false
  );

  t.end();
});

