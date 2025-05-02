/*---
features: [DataView, ArrayBuffer, BigInt, immutable-arraybuffer]
---*/

var buffer = new ArrayBuffer(1);
var sample = new DataView(buffer.transferToImmutable(), 0);

assert.throws(TypeError, function() {
  sample.setBigInt64(0, 0n);
});
