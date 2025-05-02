/*---
includes: [testTypedArray.js]
features: [TypedArray, immutable-arraybuffer]
---*/

var comparefn = function() {
  throw new Test262Error();
};

testWithTypedArrayConstructors(function(TA) {
  var buffer = new ArrayBuffer(2 * TA.BYTES_PER_ELEMENT);
  var sample = new TA(buffer.transferToImmutable());
  assert.throws(TypeError, function() {
    sample.sort(comparefn);
  });
});
