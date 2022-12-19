/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Maps', { skip: typeof Map !== 'function' }, function (t) {
  t.deepEqualTest(
    new Map([[null, undefined]]),
    new Map([[null, null]]),
    'null keys, nullish values, loosely equal, strictly inequal',
    true,
    false
  );

  t.deepEqualTest(
    new Map([[undefined, 3]]),
    new Map([[null, 3]]),
    'nullish keys, loosely equal, strictly inequal',
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

  t.deepEqualTest(
    new Map(),
    new Map([[{}, 1]]),
    'two inequal Maps',
    false,
    false
  );

  t.deepEqualTest(
    new Map([[{}, null], [false, 3]]),
    new Map([[{}, null], [true, 2]]),
    'two inequal maps, same size, primitive key, start with object key',
    false,
    false
  );

  t.deepEqualTest(
    new Map([[false, 3], [{}, null]]),
    new Map([[true, 2], [{}, null]]),
    'two inequal maps, same size, primitive key, start with primitive key',
    false,
    false
  );

  t.deepEqualTest(
    new Map([[undefined, null], ['+000', 2]]),
    new Map([[null, undefined], [false, '2']]),
    'primitive comparisons',
    true,
    false
  );

  t.end();
});

