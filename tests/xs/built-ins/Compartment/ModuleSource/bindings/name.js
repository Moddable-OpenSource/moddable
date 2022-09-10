/*---
description:
flags: [onlyStrict]
includes: [propertyHelper.js]
---*/

var descriptor = Object.getOwnPropertyDescriptor(ModuleSource.prototype, 'bindings');

assert.sameValue(
  typeof descriptor.get,
  'function',
  'typeof descriptor.get is function'
);
assert.sameValue(
  typeof descriptor.set,
  'undefined',
  'typeof descriptor.set is undefined'
);

verifyNotEnumerable(ModuleSource.prototype, 'bindings');
verifyConfigurable(ModuleSource.prototype, 'bindings');
