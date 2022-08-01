/*---
description:
includes: [propertyHelper.js]
features: [Symbol.toStringTag]
---*/

assert.sameValue(Compartment.prototype[Symbol.toStringTag], 'Compartment');

verifyNotEnumerable(Compartment.prototype, Symbol.toStringTag);
verifyNotWritable(Compartment.prototype, Symbol.toStringTag);
verifyConfigurable(Compartment.prototype, Symbol.toStringTag);
