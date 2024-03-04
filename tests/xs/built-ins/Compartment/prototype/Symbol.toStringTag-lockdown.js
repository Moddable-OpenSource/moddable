/*---
description:
includes: [propertyHelper.js]
features: [Symbol.toStringTag]
---*/

lockdown();

verifyProperty(Compartment.prototype, Symbol.toStringTag, {
  value: 'Compartment',
  writable: false,
  enumerable: false,
  configurable: false
});
