/*---
description: adapted from https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js
flags: [module]
---*/

import { assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto, test } from "./deep-equal-test-cmp_FIXTURE.js";

test('Set and Map', { skip: !Object.defineProperty || typeof Set !== 'function' || typeof Map !== 'function' }, function (t) {
  t.deepEqualTest(
    new Set(),
    new Map(),
    'Map and Set',
    false,
    false
  );

  var maplikeSet = new Set();
  Object.defineProperty(maplikeSet, 'constructor', { enumerable: false, value: Map });
  maplikeSet.__proto__ = Map.prototype; // eslint-disable-line no-proto
  t.deepEqualTest(
    maplikeSet,
    new Map(),
    'Map-like Set, and Map',
    false,
    false
  );

  t.end();
});

