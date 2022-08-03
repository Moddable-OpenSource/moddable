import deepEqual from "deepEqual";

// https://github.com/inspect-js/node-deep-equal/blob/master/test/cmp.js

var assign = Object.assign;
var gOPDs = Object.getOwnPropertyDescriptors;
var hasSymbols = true;
var hasTypedArrays = true;
var semver = null;

var safeBuffer = typeof Buffer === 'function' ? Buffer.from && Buffer.from.length > 1 ? Buffer.from : Buffer : null;
var buffersAreTypedArrays = typeof Buffer === 'function' && new Buffer(0) instanceof Uint8Array;

var isNode = typeof process === 'object' && typeof process.version === 'string';

function tag(obj, value) {
  if (hasSymbols && Symbol.toStringTag && Object.defineProperty) {
    Object.defineProperty(obj, Symbol.toStringTag, {
      value: value
    });
  }
  return obj;
}

var hasDunderProto = [].__proto__ === Array.prototype;

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

export {
  assign, gOPDs, hasSymbols, hasTypedArrays, semver, safeBuffer, buffersAreTypedArrays, isNode, tag, hasDunderProto,
  test
};
