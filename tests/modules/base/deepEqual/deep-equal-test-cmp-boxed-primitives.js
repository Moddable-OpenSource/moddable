/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('boxed primitives', function (t) {
  t.deepEqualTest(Object(false), false, 'boxed and primitive `false`', false, false);
  t.deepEqualTest(Object(true), true, 'boxed and primitive `true`', false, false);
  t.deepEqualTest(Object(3), 3, 'boxed and primitive `3`', false, false);
  t.deepEqualTest(Object(NaN), NaN, 'boxed and primitive `NaN`', false, false);
  t.deepEqualTest(Object(''), '', 'boxed and primitive `""`', false, false);
  t.deepEqualTest(Object('str'), 'str', 'boxed and primitive `"str"`', false, false);

  t.test('symbol', { skip: !hasSymbols }, function (st) {
    var s = Symbol('');
    st.deepEqualTest(Object(s), s, 'boxed and primitive `Symbol()`', false, false);
    st.end();
  });

  t.test('bigint', { skip: typeof BigInt !== 'function' }, function (st) {
    var hhgtg = BigInt(42);
    st.deepEqualTest(Object(hhgtg), hhgtg, 'boxed and primitive `BigInt(42)`', false, false);
    st.end();
  });

  t.test('`valueOf` is called for boxed primitives', { skip: true }, function (st) {
    var a = Object(5);
    a.valueOf = function () { throw new Error('failed'); };
    var b = Object(5);
    b.valueOf = function () { throw new Error('failed'); };

    st.deepEqualTest(a, b, 'two boxed numbers with a thrower valueOf', false, false);

    st.end();
  });

  t.end();
});

