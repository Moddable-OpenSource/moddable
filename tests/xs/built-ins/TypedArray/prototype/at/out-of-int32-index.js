/*---
includes: [testTypedArray.js]
features: [TypedArray]
---*/

testWithTypedArrayConstructors(function(TA) {
  var sample = new TA([7]);
  assert.sameValue(sample.at(Infinity), undefined, "Infinity");
  assert.sameValue(sample.at(-Infinity), undefined, "-Infinity");
  assert.sameValue(sample.at(1e300), undefined, "1e300");
  assert.sameValue(sample.at(2 ** 32), undefined, "2 ** 32");
  assert.sameValue(sample.at(-(2 ** 32)), undefined, "-(2 ** 32)");
  assert.sameValue(sample.at(1), undefined, "1");
  assert.sameValue(sample.at(0), 7, "0");
  assert.sameValue(sample.at(-1), 7, "-1");
});
