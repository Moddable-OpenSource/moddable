/*---
description:
includes: [propertyHelper.js]
features: [Symbol.toStringTag]
---*/

lockdown();

assert.sameValue(Compartment.prototype[Symbol.toStringTag], 'Compartment');

verifyNotEnumerable(Compartment.prototype, Symbol.toStringTag);
verifyNotWritable(Compartment.prototype, Symbol.toStringTag);
verifyNotConfigurable(Compartment.prototype, Symbol.toStringTag);
