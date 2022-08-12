/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('buffers', { skip: typeof Buffer !== 'function' }, function (t) {
  /* eslint no-buffer-constructor: 1, new-cap: 1 */
  t.deepEqualTest(
    safeBuffer('xyz'),
    safeBuffer('xyz'),
    'buffers with same contents are equal',
    true,
    true
  );

  t.deepEqualTest(
    safeBuffer('xyz'),
    safeBuffer('xyy'),
    'buffers with same length and different contents are inequal',
    false,
    false
  );

  t.deepEqualTest(
    safeBuffer('xyz'),
    safeBuffer('xy'),
    'buffers with different length are inequal',
    false,
    false
  );

  t.deepEqualTest(
    safeBuffer('abc'),
    safeBuffer('xyz'),
    'buffers with different contents',
    false,
    false
  );

  var emptyBuffer = safeBuffer('');

  t.deepEqualTest(
    emptyBuffer,
    [],
    'empty buffer and empty array',
    false,
    false
  );

  t.test('bufferlikes', { skip: !Object.defineProperty || !hasTypedArrays }, function (st) {
    var fakeBuffer = {
      0: 'a',
      length: 1,
      __proto__: emptyBuffer.__proto__, // eslint-disable-line no-proto
      copy: emptyBuffer.copy,
      slice: emptyBuffer.slice
    };
    Object.defineProperty(fakeBuffer, '0', { enumerable: false });
    Object.defineProperty(fakeBuffer, 'length', { enumerable: false });
    Object.defineProperty(fakeBuffer, 'copy', { enumerable: false });
    Object.defineProperty(fakeBuffer, 'slice', { enumerable: false });

    st.deepEqualTest(
      safeBuffer('a'),
      fakeBuffer,
      'real buffer, and mildly fake buffer',
      false,
      false
    );

    st.test('bufferlike', { skip: buffersAreTypedArrays ? !hasSymbols || !Symbol.toStringTag : false }, function (s2t) {
      var bufferlike = buffersAreTypedArrays ? new Uint8Array() : {};
      Object.defineProperty(bufferlike, 'length', {
        enumerable: false,
        value: bufferlike.length || 0
      });
      Object.defineProperty(bufferlike, 'copy', {
        enumerable: false,
        value: emptyBuffer.copy
      });
      bufferlike.__proto__ = emptyBuffer.__proto__; // eslint-disable-line no-proto

      s2t.deepEqualTest(
        emptyBuffer,
        bufferlike,
        'empty buffer and empty bufferlike',
        true,
        true
      );
      s2t.end();
    });

    st.end();
  });

  t.end();
});

