/*---
includes: [isConstructor.js]
features: [immutable-arraybuffer, Reflect.construct]
---*/

assert(!isConstructor(ArrayBuffer.prototype.transferToImmutable), "ArrayBuffer.prototype.transferToImmutable is not a constructor");

var arrayBuffer = new ArrayBuffer(8);
assert.throws(TypeError, function() {
  new arrayBuffer.transfer();
});
