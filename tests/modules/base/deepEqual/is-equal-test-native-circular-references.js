/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

var Circular = function Circular() {
	this.circularRef = this;
};

test('circular references', function (t) {
	var a = new Circular();
	var b = new Circular();
	t.equal(isEqual(a, b), true, 'two circular referencing instances are equal');

	var c = {};
	var d = {};
	c.c = c;
	d.d = d;
	t.equal(isEqual(c, d), false, 'two objects with different circular references are not equal');

	t.end();
});
