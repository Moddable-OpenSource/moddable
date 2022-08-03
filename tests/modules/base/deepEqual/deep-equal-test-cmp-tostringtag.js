/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('toStringTag', { skip: !hasSymbols || !Symbol.toStringTag }, function (t) {
  var o1 = {};
  t.equal(Object.prototype.toString.call(o1), '[object Object]', 'o1: Symbol.toStringTag works');

  var o2 = {};
  t.equal(Object.prototype.toString.call(o2), '[object Object]', 'o2: original Symbol.toStringTag works');

  t.deepEqualTest(o1, o2, 'two normal empty objects', true, true);

  o2[Symbol.toStringTag] = 'jifasnif';
  t.equal(Object.prototype.toString.call(o2), '[object jifasnif]', 'o2: modified Symbol.toStringTag works');

  t.deepEqualTest(o1, o2, 'two normal empty objects with different toStringTags', false, false);

  t.end();
});

