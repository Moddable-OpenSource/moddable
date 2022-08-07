/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('iterables', function (t) {
	t.test('Maps', { skip: typeof Map !== 'function' }, function (mt) {
		var a = new Map();
		a.set('a', 'b');
		a.set('c', 'd');
		var b = new Map();
		b.set('a', 'b');
		b.set('c', 'd');
		var c = new Map();
		c.set('a', 'b');

		mt.equal(isEqual(a, b), true, 'equal Maps (a, b) are equal');
		mt.equal(isEqual(b, a), true, 'equal Maps (b, a) are equal');
		mt.equal(isEqual(a, c), false, 'unequal Maps (a, c) are not equal');
		mt.equal(isEqual(b, c), false, 'unequal Maps (b, c) are not equal');
		mt.equal(isEqual(c, a), false, 'unequal Maps (c, a) are not equal');
		mt.equal(isEqual(c, b), false, 'unequal Maps (c, b) are not equal');

		mt.end();
	});

	t.test('Sets', { skip: typeof Set !== 'function' }, function (st) {
		var a = new Set();
		a.add('a');
		a.add('b');
		var b = new Set();
		b.add('a');
		b.add('b');
		var c = new Set();
		c.add('a');

		st.ok(isEqual(a, b), 'equal Set (a, b) are equal');
		st.ok(isEqual(b, a), 'equal Set (b, a) are equal');
		st.notOk(isEqual(a, c), 'unequal Set (a, c) are not equal');
		st.notOk(isEqual(b, c), 'unequal Set (b, c) are not equal');
		st.notOk(isEqual(c, a), 'unequal Set (c, a) are not equal');
		st.notOk(isEqual(c, b), 'unequal Set (c, b) are not equal');

		st.test('Sets with strings as iterables', function (sst) {
			var ab;
			// eslint-disable-next-line max-statements-per-line
			try { ab = new Set('ab'); } catch (e) { ab = new Set(); } // node 0.12 throws when given a string
			if (ab.size !== 2) {
				// work around IE 11 (and others) bug accepting iterables
				ab.add('a');
				ab.add('b');
			}
			var ac;
			// eslint-disable-next-line max-statements-per-line
			try { ac = new Set('ac'); } catch (e) { ac = new Set(); } // node 0.12 throws when given a string
			if (ab.size !== 2) {
				// work around IE 11 (and others) bug accepting iterables
				ab.add('a');
				ab.add('c');
			}
			sst.notOk(isEqual(ab, ac), 'Sets initially populated with different strings are not equal');
			sst.end();
		});

		st.end();
	});

	var obj = { a: { aa: true }, b: [2] };
	t.test('generic iterables', { skip: !symbolIterator }, function (it) {
		var a = { foo: 'bar' };
		var b = { bar: 'baz' };

		it.equal(isEqual(a, b), false, 'normal a and normal b are not equal');

		a[symbolIterator] = genericIterator(obj);
		it.equal(isEqual(a, b), false, 'iterable a / normal b are not equal');
		it.equal(isEqual(b, a), false, 'iterable b / normal a are not equal');
		it.equal(isEqual(a, obj), false, 'iterable a / normal obj are not equal');
		it.equal(isEqual(obj, a), false, 'normal obj / iterable a are not equal');

		b[symbolIterator] = genericIterator(obj);
		it.equal(isEqual(a, b), true, 'iterable a / iterable b are equal');
		it.equal(isEqual(b, a), true, 'iterable b / iterable a are equal');
		it.equal(isEqual(b, obj), false, 'iterable b and normal obj are not equal');
		it.equal(isEqual(obj, b), false, 'normal obj / iterable b are not equal');

		it.end();
	});

	t.test('unequal iterables', { skip: !symbolIterator }, function (it) {
		var c = {};
		c[symbolIterator] = genericIterator({});
		var d = {};
		d[symbolIterator] = genericIterator(obj);

		it.equal(isEqual(c, d), false, 'iterable c / iterable d are not equal');
		it.equal(isEqual(d, c), false, 'iterable d / iterable c are not equal');

		it.end();
	});

	t.end();
});
