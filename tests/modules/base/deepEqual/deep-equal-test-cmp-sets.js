/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Sets', { skip: typeof Set !== 'function' }, function (t) {
  t.deepEqualTest(
    new Set(['a', 1, 'b', 2]),
    new Set(['b', 2, 'a', 1]),
    'two equal Sets',
    true,
    true
  );

  t.deepEqualTest(
    new Set(['a', 1]),
    new Set(['b', 1]),
    'two inequal Sets',
    false,
    false
  );

  t.deepEqualTest(
    new Set([{}, 1, {}, {}, 2]),
    new Set([{}, 1, {}, 2, {}]),
    'two equal Sets in different orders',
    true,
    true
  );

  t.deepEqualTest(
    new Set(),
    new Set([1]),
    'two inequally sized Sets',
    false,
    false
  );

  t.deepEqualTest(
    new Set([{ a: 1 }, 2]),
    new Set(['2', { a: '1' }]),
    'two loosely equal, strictly inequal Sets',
    true,
    false
  );

  t.deepEqualTest(
    new Set([{ a: 1 }, 2]),
    new Set(['2', { a: 2 }]),
    'two inequal Sets',
    false,
    false
  );

  t.deepEqualTest(
    new Set([null, '', 1, 5, 2, false]),
    new Set([undefined, 0, '5', true, '2', '-000']),
    'more primitive comparisons',
    true,
    false
  );

  t.end();
});

