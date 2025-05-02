/*---
features: [ArrayBuffer, immutable-arraybuffer]
---*/

var ab = new ArrayBuffer(1);

assert.sameValue(ab.immutable, false);

ab = ab.transferToImmutable();

assert.sameValue(ab.immutable, true);

ab = new ArrayBuffer(1, { maxByteLength: 2 });

assert.sameValue(ab.immutable, false);

ab = ab.transferToImmutable();

assert.sameValue(ab.immutable, true);
