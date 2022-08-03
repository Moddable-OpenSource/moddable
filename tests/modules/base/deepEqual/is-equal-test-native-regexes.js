/*---
description: adapted from https://github.com/inspect-js/is-equal/blob/main/test/native.js
flags: [module]
---*/

import { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test } from "./is-equal-test-native_FIXTURE.js";

test('regexes', function (t) {
	t.ok(isEqual(/a/g, /a/g), 'two regex literals are equal');
	t.notOk(isEqual(/a/g, /b/g), 'two different regex literals (same flags, diff source) are not equal');
	t.notOk(isEqual(/a/i, /a/g), 'two different regex literals (same source, diff flags) are not equal');
	t.ok(isEqual(new RegExp('a', 'g'), new RegExp('a', 'g')), 'two regex objects are equal');
	t.notOk(isEqual(new RegExp('a', 'g'), new RegExp('b', 'g')), 'two different regex objects are equal');
	t.ok(isEqual(new RegExp('a', 'g'), /a/g), 'regex object and literal, same content, are equal');
	t.notOk(isEqual(new RegExp('a', 'g'), /b/g), 'regex object and literal, different content, are not equal');
	t.end();
});

