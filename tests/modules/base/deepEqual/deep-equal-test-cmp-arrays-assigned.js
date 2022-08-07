/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('arrays assigned', function (t) {
  var a0 = [
    undefined,
    null,
    -1,
    0,
    1,
    false,
    true,
    undefined,
    '',
    'abc',
    null,
    undefined
  ];
  var a1 = [];

  a1[0] = undefined;
  a1[1] = null;
  a1[2] = -1;
  a1[3] = 0;
  a1[4] = 1;
  a1[5] = false;
  a1[6] = true;
  a1[7] = undefined;
  a1[8] = '';
  a1[9] = 'abc';
  a1[10] = null;
  a1[11] = undefined;
  a1.length = 12;

  t.deepEqualTest(a0, a1, 'a literal array and an assigned array', true, true);
  t.end();
});

