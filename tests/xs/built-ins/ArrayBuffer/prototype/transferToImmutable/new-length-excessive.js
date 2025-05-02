/*---
features: [immutable-arraybuffer]
---*/

var ab = new ArrayBuffer(0);

assert.throws(RangeError, function() {
  // Math.pow(2, 53) = 9007199254740992
  ab.transferToImmutable(9007199254740992);
});
