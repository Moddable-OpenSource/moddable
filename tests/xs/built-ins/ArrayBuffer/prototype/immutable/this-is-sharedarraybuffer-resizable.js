/*---
features: [SharedArrayBuffer, ArrayBuffer, immutable-arraybuffer]
---*/

var immutable = Object.getOwnPropertyDescriptor(
  ArrayBuffer.prototype, "immutable"
);

var getter = immutable.get;
var sab = new SharedArrayBuffer(4);

assert.sameValue(typeof getter, "function");

assert.throws(TypeError, function() {
  getter.call(sab);
}, "`this` cannot be a SharedArrayBuffer");
