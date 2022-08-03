/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('symbols', { skip: !hasSymbols }, function (t) {
	var foo = 'foo';
	var fooSym = Symbol(foo);
	var objectFooSym = Object(fooSym);
	t.ok(isEqual(fooSym, fooSym), 'Symbol("foo") is equal to itself');

	t.test('symbols (boxed)', { skip:true }, function (st) {
		st.ok(isEqual(fooSym, fooSym), 'Symbol("foo") is equal to itself');
		st.end();
	});

	t.notOk(isEqual(Symbol(foo), Symbol(foo)), 'Symbol("foo") is not equal to Symbol("foo"), even when the string is the same instance');
	t.notOk(isEqual(Symbol(foo), Object(Symbol(foo))), 'Symbol("foo") is not equal to Object(Symbol("foo")), even when the string is the same instance');

	t.test('arrays containing symbols', function (st) {
		st.ok(
			isEqual([fooSym], [fooSym]),
			'Arrays each containing the same instance of Symbol("foo") are equal'
		);

		st.notOk(
			isEqual([Symbol(foo)], [Object(Symbol(foo))]),
			'An array containing Symbol("foo") is not equal to Object(Symbol("foo")), even when the string is the same instance'
		);

		st.end();
	});

	t.end();
});

