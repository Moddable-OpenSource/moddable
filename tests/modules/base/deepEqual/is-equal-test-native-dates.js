/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('dates', function (t) {
	t.ok(isEqual(new Date(123), new Date(123)), 'two dates with the same timestamp are equal');
	t.notOk(isEqual(new Date(123), new Date(456)), 'two dates with different timestamp are not equal');
	t.end();
});

