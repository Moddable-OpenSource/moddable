/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('functions', { skip:true }, function (t) {
	var f1 = Object(function f() { /* SOME STUFF */ return 1; });
	var f2 = Object(function f() { /* SOME STUFF */ return 1; });
	var f3 = Object(function f() { /* SOME DIFFERENT STUFF */ return 2; });
	var g = Object(function g() { /* SOME STUFF */ return 1; });
	var anon1 = Object(function () { /* ANONYMOUS! */ return 'anon'; });
	var anon2 = Object(function () { /* ANONYMOUS! */ return 'anon'; });
	/* jscs: disable */
	/* eslint-disable space-before-function-paren */
	/* eslint-disable space-before-blocks */
	var fnNoSpace = Object(function(){});
	/* eslint-enable space-before-blocks */
	/* eslint-enable space-before-function-paren */
	/* jscs: enable */
	var fnWithSpaceBeforeBody = Object(function () {});
	var emptyFnWithName = Object(function a() {});
	/* eslint-disable no-unused-vars */
	var emptyFnOneArg = Object(function (a) {});
	var anon1withArg = Object(function (a) { /* ANONYMOUS! */ return 'anon'; });
	/* eslint-enable no-unused-vars */

	/* for code coverage */
	f1();
	f2();
	f3();
	g();
	anon1();
	anon2();
	/* end for code coverage */

	t.ok(isEqual(f1, f1), 'same function is equal to itself');
	t.ok(isEqual(anon1, anon1), 'same anon function is equal to itself');
	t.notOk(isEqual(anon1, anon1withArg), 'similar anon function with different lengths are not equal');

	if (functionsHaveNames) {
		t.notOk(isEqual(f1, g), 'functions with different names but same implementations are not equal');
	} else {
		t.comment('* function names not supported *');
		t.ok(isEqual(f1, g), 'functions with different names but same implementations should not be equal, but are');
	}
	t.ok(isEqual(f1, f2), 'functions with same names but same implementations are equal');
	t.notOk(isEqual(f1, f3), 'functions with same names but different implementations are not equal');
	t.ok(isEqual(anon1, anon2), 'anon functions with same implementations are equal');

	t.ok(isEqual(fnNoSpace, fnWithSpaceBeforeBody), 'functions with same arity/name/body are equal despite whitespace between signature and body');
	if (functionsHaveNames) {
		t.notOk(isEqual(emptyFnWithName, fnNoSpace), 'functions with same arity/body, diff name, are not equal');
	} else {
		t.comment('* function names not supported *');
		t.notOk(isEqual(emptyFnWithName, fnNoSpace), 'functions with same arity/body, diff name, should not be equal, but are');
	}
	t.notOk(isEqual(emptyFnOneArg, fnNoSpace), 'functions with same name/body, diff arity, are not equal');

	t.test('generators', { skip: !hasGeneratorSupport }, function (st) {
		/* eslint-disable no-new-func */
		var genFnStar = Function('return function* () {};')();
		var genFnSpaceStar = Function('return function *() {};')();
		var genNoSpaces = Function('return function*(){};')();
		st.notOk(isEqual(fnNoSpace, genNoSpaces), 'generator and fn that are otherwise identical are not equal');

		forEach(v.generatorFunctions.concat(genFnStar, genFnSpaceStar, genNoSpaces), function (generator) {
			st.ok(isEqual(generator, generator), generator + ' is equal to itself');

			var copied = copyFunction(generator);
			st.ok(isEqual(generator, copied), inspect(generator) + ' is equal to copyFunction(' + inspect(generator) + ')');
			st.ok(isEqual(copied, generator), 'copyFunction(' + inspect(generator) + ') is equal to ' + inspect(generator));
		});

		st.end();
	});

	t.test('arrow functions', { skip: !hasArrowFunctionSupport }, function (st) {
		forEach(arrowFunctions, function (fn) {
			st.notOk(isEqual(fnNoSpace, fn), fn + ' not equal to ' + fnNoSpace);
			st.ok(isEqual(fn, fn), fn + ' equal to itself');
			st.ok(isEqual(fn, copyFunction(fn)), fn + ' equal to copyFunction(fn)');
		});
		st.end();
	});

	t.end();
});

