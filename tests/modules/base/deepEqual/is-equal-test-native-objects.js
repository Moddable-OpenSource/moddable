/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('objects', function (t) {
	t.test('prototypes', function (st) {
		var F = function F() {
			this.foo = 42;
		};
		var G = function G() {};
		G.prototype = new F();
		G.prototype.constructor = G;
		var H = function H() {};
		H.prototype = G.prototype;
		var I = function I() {};

		var f1 = new F();
		var f2 = new F();
		var g1 = new G();
		var h1 = new H();
		var i1 = new I();

		st.ok(isEqual(f1, f2), 'two instances of the same thing are equal');

		st.ok(isEqual(g1, h1), 'two instances of different things with the same prototype are equal');
		st.notOk(isEqual(f1, i1), 'two instances of different things with a different prototype are not equal');

		var isParentEqualToChild = isEqual(f1, g1);
		st.notOk(isParentEqualToChild, 'two instances of a parent and child are not equal');
		var isChildEqualToParent = isEqual(g1, f1);
		st.notOk(isChildEqualToParent, 'two instances of a child and parent are not equal');

		g1.foo = 'bar';
		var g2 = new G();
		st.notOk(isEqual(g1, g2), 'two instances of the same thing with different properties are not equal');
		st.notOk(isEqual(g2, g1), 'two instances of the same thing with different properties are not equal');
		st.end();
	});

	t.test('literals', function (st) {
		var a = { foo: 42 };
		var b = { foo: 42 };
		st.ok(isEqual(a, a), 'same hash is equal to itself');
		st.ok(isEqual(a, b), 'two similar hashes are equal');
		st.ok(isEqual({ nested: a }, { nested: a }), 'similar hashes with same nested hash are equal');
		st.ok(isEqual({ nested: a }, { nested: b }), 'similar hashes with similar nested hash are equal');

		st.notOk(isEqual({ a: 42, b: 0 }, { a: 42 }), 'second hash missing a key is not equal');
		st.notOk(isEqual({ a: 42 }, { a: 42, b: 0 }), 'first hash missing a key is not equal');

		st.notOk(isEqual({ a: 1 }, { a: 2 }), 'two objects with equal keys but inequal values are not equal');
		st.notOk(isEqual({ c: 1 }, { a: 1 }), 'two objects with inequal keys but same values are not equal');

		var obj1 = { a: 0, b: '1', c: false };
		var obj2 = { a: 0, b: '1', c: true };
		st.notOk(isEqual(obj1, obj2), 'two objects with inequal boolean keys are not equal');
		st.end();
	});

	t.test('key ordering', function (st) {
		var a = { a: 1, b: 2 };
		var b = { b: 2 };
		b.a = 1;
		st.ok(isEqual(a, b), 'objects with different key orderings but same contents are equal');
		st.end();
	});

	t.end();
});

