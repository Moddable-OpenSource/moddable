/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('arrays', function (t) {
	t.ok(isEqual([], []), 'empty arrays are equal');
	t.ok(isEqual([1, 2, 3], [1, 2, 3]), 'same arrays are equal');
	t.notOk(isEqual([1, 2, 3], [3, 2, 1]), 'arrays in different order with same values are not equal');
	t.notOk(isEqual([1, 2], [1, 2, 3]), 'arrays with different lengths are not equal');
	t.notOk(isEqual([1, 2, 3], [1, 2]), 'arrays with different lengths are not equal');

	t.test('nested values', function (st) {
		st.ok(isEqual([[1, 2], [2, 3], [3, 4]], [[1, 2], [2, 3], [3, 4]]), 'arrays with same array values are equal');
		st.end();
	});

	t.test('nested objects', function (st) {
		var arr1 = [
			{ a: 0, b: '1', c: false },
			{ a: 1, b: '2', c: false }
		];
		var arr2 = [
			{ a: 0, b: '1', c: true },
			{ a: 1, b: '2', c: false }
		];
		st.notOk(isEqual(arr1[0], arr2[0]), 'array items 0 are not equal');
		st.ok(isEqual(arr1[1], arr2[1]), 'array items 1 are equal');
		st.notOk(isEqual(arr1, arr2), 'two arrays with nested inequal objects are not equal');

		st.end();
	});

	t.end();
});

