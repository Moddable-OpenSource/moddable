/*---
features: [ArrayBuffer, immutable-arraybuffer]
---*/

assert.throws(TypeError, function() {
  ArrayBuffer.prototype.immutable;
});
