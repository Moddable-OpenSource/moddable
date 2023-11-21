/*---
description:
includes: [propertyHelper.js]
features: [Symbol.toStringTag]
---*/

verifyProperty(Compartment.prototype, Symbol.toStringTag, {
  value: 'Compartment',
  writable: false,
  enumerable: false,
  configurable: true
});
