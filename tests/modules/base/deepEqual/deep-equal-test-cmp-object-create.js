/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Object.create', { skip: !Object.create }, function (t) {
  var a = { a: 'A' };
  var b = Object.create(a);
  b.b = 'B';
  var c = Object.create(a);
  c.b = 'C';

  t.deepEqualTest(
    b,
    c,
    'two objects with the same [[Prototype]] but a different own property',
    false,
    false
  );

  t.end();
});

