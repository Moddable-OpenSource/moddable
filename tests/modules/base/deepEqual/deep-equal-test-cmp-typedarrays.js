/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('TypedArrays', { skip: !hasTypedArrays }, function (t) {
  t.test('Buffer faked as Uint8Array', { skip: typeof Buffer !== 'function' || !Object.create || !hasDunderProto || isNode06 }, function (st) {
    var a = safeBuffer('test');
    var b = tag(Object.create(
      a.__proto__, // eslint-disable-line no-proto
      assign(gOPDs(a), {
        length: {
          enumerable: false,
          value: 4
        }
      })
    ), 'Uint8Array');

    st.deepEqualTest(
      a,
      b,
      'Buffer and Uint8Array',
      isNodeWhereBufferBreaks,
      isNodeWhereBufferBreaks
    );

    st.end();
  });

  t.test('one TypedArray faking as another', { skip: !hasDunderProto }, function (st) {
    /* globals Uint8Array, Int8Array */
    var a = new Uint8Array(10);
    var b = tag(new Int8Array(10), 'Uint8Array');
    b.__proto__ = Uint8Array.prototype; // eslint-disable-line no-proto

    st.deepEqualTest(
      a,
      b,
      'Uint8Array, and Int8Array pretending to be a Uint8Array',
      false,
      false
    );

    st.end();
  });

  t.test('ArrayBuffers', { skip: typeof ArrayBuffer !== 'function' }, function (st) {
    var buffer1 = new ArrayBuffer(8); // initial value of 0's
    var buffer2 = new ArrayBuffer(8); // initial value of 0's

    var view1 = new Int8Array(buffer1);
    var view2 = new Int8Array(buffer2);

    st.deepEqualTest(
      view1,
      view2,
      'Int8Arrays of similar ArrayBuffers',
      true,
      true
    );

    st.deepEqualTest(
      buffer1,
      buffer2,
      'similar ArrayBuffers',
      true,
      true
    );

    for (var i = 0; i < view1.byteLength; i += 1) {
      view1[i] = 9; // change all values to 9's
    }

    st.deepEqualTest(
      view1,
      view2,
      'Int8Arrays of different ArrayBuffers',
      false,
      false
    );

    st.deepEqualTest(
      buffer1,
      buffer2,
      'different ArrayBuffers',
      false,
      false
    );

    t.test('lies about byteLength', { skip: !('byteLength' in ArrayBuffer.prototype) }, function (s2t) {
      var empty4 = new ArrayBuffer(4);
      var empty6 = new ArrayBuffer(6);
      Object.defineProperty(empty6, 'byteLength', { value: 4 });

      s2t.deepEqualTest(
        empty4,
        empty6,
        'different-length ArrayBuffers, one lying',
        false,
        false
      );
      s2t.end();
    });

    st.end();
  });

  t.end();
});
