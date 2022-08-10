/*---
description: adapted from https://github.com/web-platform-tests/wpt/tree/master/html/webappapis/structured-clone
flags: [module]
---*/

import { structuredClone, self, assert_equals, assert_false, assert_not_equals, assert_true, assert_unreached, check, compare_primitive, compare_Array, compare_Object, enumerate_props } from "./webappapis-structured-clone_FIXTURE.js";

function compare_obj(what) {
  const Type = self[what];
  return function(actual, input) {
    if (typeof actual === 'string')
      assert_unreached(actual);
    assert_true(actual instanceof Type, 'instanceof '+what);
    assert_equals(Type(actual), Type(input), 'converted to primitive');
    assert_not_equals(actual, input);
  };
}
check('String empty string', new String(''), compare_obj('String'));
check('String lone high surrogate', new String('\uD800'), compare_obj('String'));
check('String lone low surrogate', new String('\uDC00'), compare_obj('String'));
check('String NUL', new String('\u0000'), compare_obj('String'));
check('String astral character', new String('\uDBFF\uDFFD'), compare_obj('String'));
check('Array String objects', [new String(''),
                               new String('\uD800'),
                               new String('\uDC00'),
                               new String('\u0000'),
                               new String('\uDBFF\uDFFD')], compare_Array(enumerate_props(compare_obj('String'))));
check('Object String objects', {'empty':new String(''),
                               'high surrogate':new String('\uD800'),
                               'low surrogate':new String('\uDC00'),
                               'nul':new String('\u0000'),
                               'astral':new String('\uDBFF\uDFFD')}, compare_Object(enumerate_props(compare_obj('String'))));


check('Number 0.2', new Number(0.2), compare_obj('Number'));
check('Number 0', new Number(0), compare_obj('Number'));
check('Number -0', new Number(-0), compare_obj('Number'));
check('Number NaN', new Number(NaN), compare_obj('Number'));
check('Number Infinity', new Number(Infinity), compare_obj('Number'));
check('Number -Infinity', new Number(-Infinity), compare_obj('Number'));
check('Number 9007199254740992', new Number(9007199254740992), compare_obj('Number'));
check('Number -9007199254740992', new Number(-9007199254740992), compare_obj('Number'));
check('Number 9007199254740994', new Number(9007199254740994), compare_obj('Number'));
check('Number -9007199254740994', new Number(-9007199254740994), compare_obj('Number'));
// BigInt does not have a non-throwing constructor
check('BigInt -9007199254740994n', Object(-9007199254740994n), compare_obj('BigInt'));

check('Array Number objects', [new Number(0.2),
                               new Number(0),
                               new Number(-0),
                               new Number(NaN),
                               new Number(Infinity),
                               new Number(-Infinity),
                               new Number(9007199254740992),
                               new Number(-9007199254740992),
                               new Number(9007199254740994),
                               new Number(-9007199254740994)], compare_Array(enumerate_props(compare_obj('Number'))));
check('Object Number objects', {'0.2':new Number(0.2),
                               '0':new Number(0),
                               '-0':new Number(-0),
                               'NaN':new Number(NaN),
                               'Infinity':new Number(Infinity),
                               '-Infinity':new Number(-Infinity),
                               '9007199254740992':new Number(9007199254740992),
                               '-9007199254740992':new Number(-9007199254740992),
                               '9007199254740994':new Number(9007199254740994),
                               '-9007199254740994':new Number(-9007199254740994)}, compare_Object(enumerate_props(compare_obj('Number'))));
