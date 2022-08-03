/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('WeakSets', { skip: true }, function (t) {
  t.deepEqualTest(
    new WeakSet([Object, Function]),
    new WeakSet([Function, Object]),
    'two equal WeakSets',
    true,
    true
  );

  t.deepEqualTest(
    new WeakSet([Object, Function]),
    new WeakSet([Object]),
    'two inequal WeakSets',
    true,
    true
  );

  t.end();
});

