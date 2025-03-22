/*---
includes: [propertyHelper.js]
features: [immutable-arraybuffer]
---*/

verifyProperty(ArrayBuffer.prototype, 'transferToImmutable', {
  enumerable: false,
  writable: true,
  configurable: true
});
