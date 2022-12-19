/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('regexen', function (t) {
  t.deepEqualTest(/abc/, /xyz/, 'two different regexes', false, false);
  t.deepEqualTest(/abc/, /abc/, 'two abc regexes', true, true, false);
  t.deepEqualTest(/xyz/, /xyz/, 'two xyz regexes', true, true, false);

  t.test('fake RegExp', { skip: !hasDunderProto }, function (st) {
    var a = /abc/g;
    var b = tag(Object.create(
      a.__proto__, // eslint-disable-line no-proto
      gOPDs(a)
    ), 'RegExp');

    st.deepEqualTest(a, b, 'regex and fake regex', false, false);

    st.end();
  });

  var a = /abc/gi;
  var b = /abc/gi;
  b.foo = true;
  t.deepEqualTest(
    a,
    b,
    'two identical regexes, one with an extra property',
    false,
    false
  );

  var c = /abc/g;
  var d = /abc/i;
  t.deepEqualTest(
    c,
    d,
    'two regexes with the same source but different flags',
    false,
    false
  );

  t.end();
});

