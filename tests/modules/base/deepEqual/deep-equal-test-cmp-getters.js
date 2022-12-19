/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('getters', { skip: !Object.defineProperty }, function (t) {
  var a = {};
  Object.defineProperty(a, 'a', { enumerable: true, get: function () { return 5; } });
  var b = {};
  Object.defineProperty(b, 'a', { enumerable: true, get: function () { return 6; } });

  t.deepEqualTest(a, b, 'two objects with the same getter but producing different values', false, false);

  t.end();
});

