/*---
includes: [propertyHelper.js]
features: [ArrayBuffer, immutable-arraybuffer]
---*/

var desc = Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'immutable');

verifyProperty(desc.get, 'length', {
  value: 0,
  enumerable: false,
  writable: false,
  configurable: true
});
