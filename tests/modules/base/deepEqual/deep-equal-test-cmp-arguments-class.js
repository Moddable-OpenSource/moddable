/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('arguments class', function (t) {
  function getArgs() {
    return arguments;
  }

  t.deepEqualTest(
    getArgs(1, 2, 3),
    getArgs(1, 2, 3),
    'equivalent arguments objects are equal',
    true,
    true,
    true
  );

  t.deepEqualTest(
    getArgs(1, 2, 3),
    [1, 2, 3],
    'array and arguments with same contents',
    false,
    false
  );

  var args = getArgs();
  var notArgs = tag({ length: 0 }, 'Arguments');
  t.deepEqualTest(
    args,
    notArgs,
    'args and similar arraylike object',
    false,
    false
  );

  t.end();
});

