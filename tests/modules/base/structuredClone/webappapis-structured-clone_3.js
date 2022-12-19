/*---
description: adapted from https://github.com/web-platform-tests/wpt/tree/master/html/webappapis/structured-clone
flags: [module]
---*/

import { structuredClone, self, assert_equals, assert_false, assert_not_equals, assert_true, assert_unreached, check, compare_primitive, compare_Array, compare_Object, enumerate_props } from "./webappapis-structured-clone_FIXTURE.js";

check('Array sparse', new Array(10), compare_Array(enumerate_props(compare_primitive)));
check('Array with non-index property', function() {
  const rv = [];
  rv.foo = 'bar';
  return rv;
}, compare_Array(enumerate_props(compare_primitive)));
check('Object with index property and length', {'0':'foo', 'length':1}, compare_Object(enumerate_props(compare_primitive)));
function check_circular_property(prop) {
  return function(actual) {
    assert_equals(actual[prop], actual);
  };
}
check('Array with circular reference', function() {
  const rv = [];
  rv[0] = rv;
  return rv;
}, compare_Array(check_circular_property('0')));
check('Object with circular reference', function() {
  const rv = {};
  rv['x'] = rv;
  return rv;
}, compare_Object(check_circular_property('x')));
function check_identical_property_values(prop1, prop2) {
  return function(actual) {
    assert_equals(actual[prop1], actual[prop2]);
  };
}
check('Array with identical property values', function() {
  const obj = {}
  return [obj, obj];
}, compare_Array(check_identical_property_values('0', '1')));
check('Object with identical property values', function() {
  const obj = {}
  return {'x':obj, 'y':obj};
}, compare_Object(check_identical_property_values('x', 'y')));

function check_absent_property(prop) {
  return function(actual) {
    assert_false(prop in actual);
  };
}
check('Object with property on prototype', function() {
  const Foo = function() {};
  Foo.prototype = {'foo':'bar'};
  return new Foo();
}, compare_Object(check_absent_property('foo')));

check('Object with non-enumerable property', function() {
  const rv = {};
  Object.defineProperty(rv, 'foo', {value:'bar', enumerable:false, writable:true, configurable:true});
  return rv;
}, compare_Object(check_absent_property('foo')));

function check_writable_property(prop) {
  return function(actual, input) {
    assert_equals(actual[prop], input[prop]);
    actual[prop] += ' baz';
    assert_equals(actual[prop], input[prop] + ' baz');
  };
}
check('Object with non-writable property', function() {
  const rv = {};
  Object.defineProperty(rv, 'foo', {value:'bar', enumerable:true, writable:false, configurable:true});
  return rv;
}, compare_Object(check_writable_property('foo')));

function check_configurable_property(prop) {
  return function(actual, input) {
    assert_equals(actual[prop], input[prop]);
    delete actual[prop];
    assert_false('prop' in actual);
  };
}
check('Object with non-configurable property', function() {
  const rv = {};
  Object.defineProperty(rv, 'foo', {value:'bar', enumerable:true, writable:true, configurable:false});
  return rv;
}, compare_Object(check_configurable_property('foo')));

check('ObjectPrototype must lose its exotic-ness when cloned',
  () => Object.prototype,
  (copy, original) => {
    assert_not_equals(copy, original);
    assert_true(copy instanceof Object);

    const newProto = { some: 'proto' };
    // Must not throw:
    Object.setPrototypeOf(copy, newProto);

    assert_equals(Object.getPrototypeOf(copy), newProto);
  }
);
