/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('non-objects', function (t) {
  t.deepEqualTest(3, 3, 'same numbers', true, true, true);
  t.deepEqualTest('beep', 'beep', 'same strings', true, true, true);
  t.deepEqualTest('3', 3, 'numeric string and number', true, false);
  t.deepEqualTest('3', [3], 'numeric string and array containing number', false, false);
  t.deepEqualTest(3, [3], 'number and array containing number', false, false);

  t.end();
});

