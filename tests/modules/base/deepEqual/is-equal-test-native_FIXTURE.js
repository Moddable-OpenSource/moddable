import deepEqual from "deepEqual";

// https://github.com/inspect-js/is-equal/blob/main/test/native.js

var hasSymbols = true;
var hasSymbolShams = false;
var hasBigInts = true;
var arrowFunctions = [
	(a, b) => a * b,
	() => 42,
	() => function () {},
	() => x => x * x,
	y => x => x * x,
	x => x * x,
	x => { return x * x; },
	(x, y) => { return x + x; },
];
var hasArrowFunctionSupport = true;
var objectEntries = Object.entries;
var forEach = function(array, fn) {
	Array.prototype.forEach.call(array, fn);
};
var functionsHaveNames = true;
var inspect = function(o) { return o.toString() };
var generatorFunctions = [];
try {
	generatorFunctions.push(Function('return function* () { var x = yield; return x || 42; }')());
} catch (e) {}
try {
	generatorFunctions.push(Function('return function* gen() { var x = yield; return x || 42; }')());
} catch (e) {}
try {
	generatorFunctions.push(Function('return { *       concise(  ){ var x = yield; return x || 42; } }.concise;')());
} catch (e) {}
var symbolIterator = false;

var copyFunction = function (fn) {
	/* eslint-disable no-new-func */
	try {
		return Function('return ' + String(fn))();
	} catch (e) {
		return Function('return {' + String(fn) + '}["' + fn.name + '"];')();
	}
};

const isEqual = deepEqual;

function test(title, options, fn) {
	if (fn === undefined) {
		fn = options;
		options = {};
	}
	const titles = [];

	const t = {
		deepEqualTest( a, b, msg, isEqual, isStrictEqual, skipReversed) {
			titles.push(msg);
			msg = titles.join(": ");
			assert.sameValue(deepEqual(a, b), isEqual, msg);
			assert.sameValue(deepEqual(a, b, { strict:true }), isStrictEqual, msg + " (strict)");
			titles.pop();
		},
		ok(result, comment) {
			titles.push(comment);
			assert(result, titles.join(": "));
			titles.pop();
		},
		notOk(result, comment) {
			titles.push(comment);
			assert(!result, titles.join(": "));
			titles.pop();
		},
		end() {
		},
		equal(actual, expected, comment) {
			titles.push(comment);
			assert.sameValue(actual, expected, titles.join(": "));
			titles.pop();
		},
		test(title, options, fn) {
			if (fn === undefined) {
				fn = options;
				options = {};
			}
			titles.push(title);
			if (options.skip) {
				print("# SKIP: " + titles.join(": "));
			}
			else
				fn(t);
			titles.pop();
		}
	}
	titles.push(title);
	if (options.skip)
		print("# SKIP: " + title);
	else
		fn(t);
}

export { hasSymbols, hasSymbolShams, hasBigInts, arrowFunctions, hasArrowFunctionSupport, objectEntries, forEach, functionsHaveNames, inspect, generatorFunctions, symbolIterator, copyFunction, isEqual, test };
