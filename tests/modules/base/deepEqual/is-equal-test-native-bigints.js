/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('bigints', { skip: !hasBigInts }, function (t) {
	var bigInt = BigInt(42);
	var objectBigInt = Object(bigInt);
	t.ok(isEqual(bigInt, bigInt), '42n is equal to itself');
// 	t.ok(isEqual(bigInt, objectBigInt), '42n is equal to the object form of itself');
	t.notOk(isEqual(bigInt, BigInt(40)), '42n !== 40n');

	t.test('arrays containing bigints', function (st) {
		st.ok(
			isEqual([bigInt], [bigInt]),
			'Arrays each containing 42n are equal'
		);

		st.ok(
			isEqual([objectBigInt], [Object(bigInt)]),
			'Arrays each containing different instances of Object(42n) are equal'
		);

		st.end();
	});

	t.test('arrays containing bigints (boxed)', { skip:true }, function (st) {
		st.ok(
			isEqual([bigInt], [objectBigInt]),
			'An array containing 42n is equal to an array containing Object(42n)'
		);

		st.end();
	});

	t.end();
});

var genericIterator = function (obj) {
	var entries = objectEntries(obj);
	return function iterator() {
		return {
			next: function () {
				return {
					done: entries.length === 0,
					value: entries.shift()
				};
			}
		};
	};
};

