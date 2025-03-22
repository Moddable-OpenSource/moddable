/*---
features: [immutable-arraybuffer]
includes: [propertyHelper.js]
---*/

verifyProperty(ArrayBuffer.prototype.transferToImmutable, 'name', {
  value: 'transferToImmutable',
  enumerable: false,
  writable: false,
  configurable: true
});
