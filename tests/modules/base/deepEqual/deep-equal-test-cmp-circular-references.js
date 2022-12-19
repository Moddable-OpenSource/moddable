/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('circular references', function (t) {
  var b = {};
  b.b = b;

  var c = {};
  c.b = c;

  t.deepEqualTest(
    b,
    c,
    'two self-referencing objects',
    true,
    true
  );

  var d = {};
  d.a = 1;
  d.b = d;

  var e = {};
  e.a = 1;
  e.b = e.a;

  t.deepEqualTest(
    d,
    e,
    'two deeply self-referencing objects',
    false,
    false
  );

  t.end();
});

// io.js v2 is the only version where `console.log(b)` below is catchable
var isNodeWhereBufferBreaks = isNode && semver.satisfies(process.version, '< 3');
var isNode06 = isNode && semver.satisfies(process.version, '<= 0.6'); // segfaults in node 0.6, it seems

