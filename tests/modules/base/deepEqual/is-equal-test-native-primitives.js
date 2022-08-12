/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('primitives', function (t) {
	t.ok(isEqual(), 'undefineds are equal');
	t.ok(isEqual(null, null), 'nulls are equal');
	t.ok(isEqual(true, true), 'trues are equal');
	t.ok(isEqual(false, false), 'falses are equal');
	t.notOk(isEqual(true, false), 'true:false is not equal');
	t.notOk(isEqual(false, true), 'false:true is not equal');
	t.ok(isEqual('foo', 'foo'), 'strings are equal');
	t.ok(isEqual(42, 42), 'numbers are equal');
	t.ok(isEqual(0 / Infinity, -0 / Infinity), 'opposite sign zeroes are equal');
	t.ok(isEqual(Infinity, Infinity), 'infinities are equal');
	t.end();
});

