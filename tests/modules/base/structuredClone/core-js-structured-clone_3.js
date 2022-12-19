/*---
description: adapted from https://github.com/zloirock/core-js/blob/master/tests/tests/web.structured-clone.js
flags: [module]
includes: [compareArray.js]
---*/

import { structuredClone, GLOBAL, NODE, from, assign, getPrototypeOf, keys, fromSource, QUnit, cloneTest, cloneObjectTest } from "./core-js-structured-clone_FIXTURE.js"

// Map
QUnit.test('Map', assert => {
  cloneObjectTest(assert, new Map([[1, 2], [3, 4]]), (orig, clone) => {
    assert.deepEqual(from(orig.keys()), from(clone.keys()));
    assert.deepEqual(from(orig.values()), from(clone.values()));
  });
});

// Set
QUnit.test('Set', assert => {
  cloneObjectTest(assert, new Set([1, 2, 3, 4]), (orig, clone) => {
    assert.deepEqual(from(orig.values()), from(clone.values()));
  });
});

// Error
QUnit.test('Error', assert => {
  const errors = [
    ['Error', new Error()],
    ['Error', new Error('abc', 'def', { cause: 42 })],
    ['EvalError', new EvalError()],
    ['EvalError', new EvalError('ghi', 'jkl', { cause: 42 })],
    ['RangeError', new RangeError()],
    ['RangeError', new RangeError('ghi', 'jkl', { cause: 42 })],
    ['ReferenceError', new ReferenceError()],
    ['ReferenceError', new ReferenceError('ghi', 'jkl', { cause: 42 })],
    ['SyntaxError', new SyntaxError()],
    ['SyntaxError', new SyntaxError('ghi', 'jkl', { cause: 42 })],
    ['TypeError', new TypeError()],
    ['TypeError', new TypeError('ghi', 'jkl', { cause: 42 })],
    ['URIError', new URIError()],
    ['URIError', new URIError('ghi', 'jkl', { cause: 42 })],
    ['AggregateError', new AggregateError([1, 2])],
    ['AggregateError', new AggregateError([1, 2], 42, { cause: 42 })],
  ];

  const compile = fromSource('WebAssembly.CompileError()');
  const link = fromSource('WebAssembly.LinkError()');
  const runtime = fromSource('WebAssembly.RuntimeError()');

  if (compile && compile.name === 'CompileError') errors.push(['CompileError', compile]);
  if (link && link.name === 'LinkError') errors.push(['LinkError', link]);
  if (runtime && runtime.name === 'RuntimeError') errors.push(['RuntimeError', runtime]);

  for (const [name, error] of errors) cloneObjectTest(assert, error, (orig, clone) => {
    assert.same(orig.constructor, clone.constructor, `${ name }#constructor`);
    assert.same(orig.name, clone.name, `${ name }#name`);
    assert.same(orig.message, clone.message, `${ name }#message`);
    assert.same(orig.stack, clone.stack, `${ name }#stack`);
    assert.same(orig.cause, clone.cause, `${ name }#cause`);
    assert.deepEqual(orig.errors, clone.errors, `${ name }#errors`);
  });
});
