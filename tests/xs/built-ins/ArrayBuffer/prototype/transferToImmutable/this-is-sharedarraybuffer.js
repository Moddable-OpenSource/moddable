/*---
features: [SharedArrayBuffer, immutable-arraybuffer]
---*/

var sab = new SharedArrayBuffer(0);

assert.throws(TypeError, function() {
  ArrayBuffer.prototype.transferToImmutable.call(sab);
}, '`this` value cannot be a SharedArrayBuffer');
