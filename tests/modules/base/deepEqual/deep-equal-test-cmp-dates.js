/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Dates', function (t) {
  var d0 = new Date(1387585278000);
  var d1 = new Date('Fri Dec 20 2013 16:21:18 GMT-0800 (PST)');

  t.deepEqualTest(d0, d1, 'two Dates with the same timestamp', true, true);

  d1.a = true;

  t.deepEqualTest(d0, d1, 'two Dates with the same timestamp but different own properties', false, false);

  t.test('overriding `getTime`', { skip: !Object.defineProperty }, function (st) {
    var a = new Date('2000');
    var b = new Date('2000');
    Object.defineProperty(a, 'getTime', { value: function () { return 5; } });
    st.deepEqualTest(a, b, 'two Dates with the same timestamp but one has overridden `getTime`', true, true);
    st.end();
  });

  t.test('fake Date', { skip: !hasDunderProto }, function (st) {
    var a = new Date(2000);
    var b = tag(Object.create(
      a.__proto__, // eslint-disable-line no-proto
      gOPDs(a)
    ), 'Date');

    st.deepEqualTest(
      a,
      b,
      'Date, and fake Date',
      false,
      false
    );

    st.end();
  });

  var a = new Date('2000');
  var b = new Date('2000');
  b.foo = true;
  t.deepEqualTest(
    a,
    b,
    'two identical Dates, one with an extra property',
    false,
    false
  );

  t.deepEqualTest(
    new Date('2000'),
    new Date('2001'),
    'two inequal Dates',
    false,
    false
  );

  t.end();
});

