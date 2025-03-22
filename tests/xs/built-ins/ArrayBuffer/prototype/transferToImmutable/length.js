/*---
includes: [propertyHelper.js]
features: [immutable-arraybuffer]
---*/

verifyProperty(ArrayBuffer.prototype.transferToImmutable, 'length', {
  value: 0,
  enumerable: false,
  writable: false,
  configurable: true
});
