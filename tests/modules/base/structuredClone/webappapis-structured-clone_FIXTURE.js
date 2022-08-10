import structuredClone from "structuredClone";

const self = globalThis;
let description = "";

function assert_equals(actual, expected, message="") {
	assert.sameValue(actual, expected, description + " " + message);
}
function assert_false(value, message="") {
	assert(!value, message);
}
function assert_not_equals(actual, expected, message="") {
	assert.notSameValue(actual, expected, message);
}
function assert_true(value, message="") {
	assert(value, message);
}
function assert_unreached(message) {
	assert(false, message);
}

function check(title, input, callback) {
	description = title;
  	let newInput = input;
  	if (typeof input === 'function') {
		newInput = input();
  	}
	const copy = structuredClone(newInput);
    callback(copy, newInput);
}

function compare_primitive(actual, input) {
  assert_equals(actual, input);
}

function compare_Array(callback) {
  return function(actual, input) {
    if (typeof actual === 'string')
      assert_unreached(actual);
    assert_true(actual instanceof Array, 'instanceof Array');
    assert_not_equals(actual, input);
    assert_equals(actual.length, input.length, 'length');
    callback(actual, input);
  }
}

function compare_Object(callback) {
  return function(actual, input) {
    if (typeof actual === 'string')
      assert_unreached(actual);
    assert_true(actual instanceof Object, 'instanceof Object');
    assert_false(actual instanceof Array, 'instanceof Array');
    assert_not_equals(actual, input);
    callback(actual, input);
  }
}

function enumerate_props(compare_func) {
  return function(actual, input) {
    for (const x in input) {
      compare_func(actual[x], input[x]);
    }
  };
}

export { structuredClone, self, assert_equals, assert_false, assert_not_equals, assert_true, assert_unreached, check, compare_primitive, compare_Array, compare_Object, enumerate_props };
