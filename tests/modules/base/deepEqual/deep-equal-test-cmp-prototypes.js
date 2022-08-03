/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('[[Prototypes]]', function (t) {
  function C() {}
  var instance = new C();
  delete instance.constructor;

  t.deepEqualTest({}, instance, 'two identical objects with different [[Prototypes]]', true, false);

  t.test('Dates with different prototypes', { skip: !hasDunderProto }, function (st) {
    var d1 = new Date(0);
    var d2 = new Date(0);

    st.deepEqualTest(d1, d2, 'two dates with the same timestamp', true, true);

    var newProto = {
      __proto__: Date.prototype
    };
    d2.__proto__ = newProto; // eslint-disable-line no-proto
    st.ok(d2 instanceof Date, 'd2 is still a Date instance after tweaking [[Prototype]]');

    st.deepEqualTest(d1, d2, 'two dates with the same timestamp and different [[Prototype]]', true, false);

    st.end();
  });

  t.end();
});

