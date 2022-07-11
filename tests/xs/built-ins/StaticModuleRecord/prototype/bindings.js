/*---
description:
flags: [onlyStrict]
includes: [propertyHelper.js]
---*/

var descriptor = Object.getOwnPropertyDescriptor(StaticModuleRecord.prototype, 'bindings');

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

verifyNotEnumerable(StaticModuleRecord.prototype, 'bindings');
verifyConfigurable(StaticModuleRecord.prototype, 'bindings');
