/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Maps', { skip: typeof Map !== 'function' }, function (t) {
  t.deepEqualTest(
    new Map([['a', 1], ['b', 2]]),
    new Map([['b', 2], ['a', 1]]),
    'two equal Maps',
    true,
    true
  );

  t.deepEqualTest(
    new Map([['a', [1, 2]]]),
    new Map([['a', [2, 1]]]),
    'two Maps with inequal values on the same key',
    false,
    false
  );

  t.deepEqualTest(
    new Map([['a', 1]]),
    new Map([['b', 1]]),
    'two inequal Maps',
    false,
    false
  );

  t.deepEqualTest(
    new Map([[{}, 3], [{}, 2], [{}, 1]]),
    new Map([[{}, 1], [{}, 2], [{}, 3]]),
    'two equal Maps in different orders with object keys',
    true,
    true
  );

  t.deepEqualTest(
    new Map([[undefined, undefined]]),
    new Map([[undefined, null]]),
    'undefined keys, nullish values, loosely equal, strictly inequal',
    true,
    false
  );

  t.deepEqualTest(
    new Map([[{}, null], [true, 2], [{}, 1], [undefined, {}]]),
    new Map([[{}, 1], [true, 2], [{}, null], [undefined, {}]]),
    'two equal Maps in different orders with primitive keys',
    true,
    true
  );

  t.deepEqualTest(
    new Map([[false, 3], [{}, 2], [{}, 1]]),
    new Map([[{}, 1], [{}, 2], [false, 3]]),
    'two equal Maps in different orders with a mix of keys',
    true,
    true
  );

  t.end();
});

