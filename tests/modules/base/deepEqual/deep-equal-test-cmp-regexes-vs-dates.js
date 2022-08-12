/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('regexes vs dates', function (t) {
  var d = new Date(1387585278000);
  var r = /abc/;

  t.deepEqualTest(d, r, 'Date and RegExp', false, false);

  t.end();
});

