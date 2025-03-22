/*---
includes: [testTypedArray.js]
features: [TypedArray, immutable-arraybuffer]
---*/

testWithTypedArrayConstructors(function(TA) {
  var buffer = new ArrayBuffer(42 * TA.BYTES_PER_ELEMENT);
  var sample = new TA(buffer.transferToImmutable());
  assert.throws(TypeError, function() {
  	sample[0] = 1;
  });
});
