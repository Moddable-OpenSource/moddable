/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('null == undefined', function (t) {
  t.deepEqualTest(null, undefined, 'null and undefined', true, false);
  t.deepEqualTest([null], [undefined], '[null] and [undefined]', true, false);

  t.end();
});

