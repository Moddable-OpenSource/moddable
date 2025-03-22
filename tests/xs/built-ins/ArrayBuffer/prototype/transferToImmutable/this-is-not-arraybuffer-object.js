/*---
features: [immutable-arraybuffer]
---*/

assert.sameValue(typeof ArrayBuffer.prototype.transferToImmutable, 'function');

assert.throws(TypeError, function() {
  ArrayBuffer.prototype.transferToImmutable();
}, '`this` value is the ArrayBuffer prototype');

assert.throws(TypeError, function() {
  ArrayBuffer.prototype.transferToImmutable.call({});
}, '`this` value is an object');

assert.throws(TypeError, function() {
  ArrayBuffer.prototype.transferToImmutable.call([]);
}, '`this` value is an array');
