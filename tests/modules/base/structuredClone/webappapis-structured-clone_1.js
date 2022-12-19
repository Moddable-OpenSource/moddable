/*---
description: adapted from https://github.com/web-platform-tests/wpt/tree/master/html/webappapis/structured-clone
flags: [module]
---*/

import { structuredClone, self, assert_equals, assert_false, assert_not_equals, assert_true, assert_unreached, check, compare_primitive, compare_Array, compare_Object, enumerate_props } from "./webappapis-structured-clone_FIXTURE.js";

check('primitive undefined', undefined, compare_primitive);
check('primitive null', null, compare_primitive);
check('primitive true', true, compare_primitive);
check('primitive false', false, compare_primitive);
check('primitive string, empty string', '', compare_primitive);
check('primitive string, lone high surrogate', '\uD800', compare_primitive);
check('primitive string, lone low surrogate', '\uDC00', compare_primitive);
check('primitive string, NUL', '\u0000', compare_primitive);
check('primitive string, astral character', '\uDBFF\uDFFD', compare_primitive);
check('primitive number, 0.2', 0.2, compare_primitive);
check('primitive number, 0', 0, compare_primitive);
check('primitive number, -0', -0, compare_primitive);
check('primitive number, NaN', NaN, compare_primitive);
check('primitive number, Infinity', Infinity, compare_primitive);
check('primitive number, -Infinity', -Infinity, compare_primitive);
check('primitive number, 9007199254740992', 9007199254740992, compare_primitive);
check('primitive number, -9007199254740992', -9007199254740992, compare_primitive);
check('primitive number, 9007199254740994', 9007199254740994, compare_primitive);
check('primitive number, -9007199254740994', -9007199254740994, compare_primitive);
check('primitive BigInt, 0n', 0n, compare_primitive);
check('primitive BigInt, -0n', -0n, compare_primitive);
check('primitive BigInt, -9007199254740994000n', -9007199254740994000n, compare_primitive);
check('primitive BigInt, -9007199254740994000900719925474099400090071992547409940009007199254740994000n', -9007199254740994000900719925474099400090071992547409940009007199254740994000n, compare_primitive);

check('Array primitives', [undefined,
                           null,
                           true,
                           false,
                           '',
                           '\uD800',
                           '\uDC00',
                           '\u0000',
                           '\uDBFF\uDFFD',
                           0.2,
                           0,
                           -0,
                           NaN,
                           Infinity,
                           -Infinity,
                           9007199254740992,
                           -9007199254740992,
                           9007199254740994,
                           -9007199254740994,
                           -12n,
                           -0n,
                           0n], compare_Array(enumerate_props(compare_primitive)));
check('Object primitives', {'undefined':undefined,
                           'null':null,
                           'true':true,
                           'false':false,
                           'empty':'',
                           'high surrogate':'\uD800',
                           'low surrogate':'\uDC00',
                           'nul':'\u0000',
                           'astral':'\uDBFF\uDFFD',
                           '0.2':0.2,
                           '0':0,
                           '-0':-0,
                           'NaN':NaN,
                           'Infinity':Infinity,
                           '-Infinity':-Infinity,
                           '9007199254740992':9007199254740992,
                           '-9007199254740992':-9007199254740992,
                           '9007199254740994':9007199254740994,
                           '-9007199254740994':-9007199254740994}, compare_Object(enumerate_props(compare_primitive)));

function compare_Boolean(actual, input) {
  if (typeof actual === 'string')
    assert_unreached(actual);
  assert_true(actual instanceof Boolean, 'instanceof Boolean');
  assert_equals(String(actual), String(input), 'converted to primitive');
  assert_not_equals(actual, input);
}
check('Boolean true', new Boolean(true), compare_Boolean);
check('Boolean false', new Boolean(false), compare_Boolean);
check('Array Boolean objects', [new Boolean(true), new Boolean(false)], compare_Array(enumerate_props(compare_Boolean)));
check('Object Boolean objects', {'true':new Boolean(true), 'false':new Boolean(false)}, compare_Object(enumerate_props(compare_Boolean)));

