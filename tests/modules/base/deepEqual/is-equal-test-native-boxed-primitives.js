/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('boxed primitives', { skip:true }, function (t) {
	t.ok(isEqual(Object(''), ''), 'Empty String and empty string are equal');
	t.ok(isEqual(Object('foo'), 'foo'), 'String and string are equal');
	t.ok(isEqual(Object(true), true), 'Boolean true and boolean true are equal');
	t.ok(isEqual(Object(false), false), 'Boolean false and boolean false are equal');
	t.ok(isEqual(true, Object(true)), 'boolean true and Boolean true are equal');
	t.ok(isEqual(false, Object(false)), 'boolean false and Boolean false are equal');
	t.notOk(isEqual(Object(true), false), 'Boolean true and boolean false are not equal');
	t.notOk(isEqual(Object(false), true), 'Boolean false and boolean true are not equal');
	t.notOk(isEqual(false, Object(true)), 'boolean false and Boolean true are not equal');
	t.notOk(isEqual(true, Object(false)), 'boolean true and Boolean false are not equal');
	t.ok(isEqual(Object(42), 42), 'Number and number literal are equal');
	t.end();
});

