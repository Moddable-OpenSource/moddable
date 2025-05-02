/*---
features: [uint8array-base64, TypedArray, immutable-arraybuffer]
---*/

var buffer = new ArrayBuffer(3);
var target = new Uint8Array(buffer.transferToImmutable());
assert.throws(TypeError, function() {
  target.setFromBase64('aa');
});
