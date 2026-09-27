/*---
description: fromBigInt writes into the start of a larger ArrayBuffer constructed by this
includes: [compareArray.js]
features: [BigInt]
---*/

function Ctor(length) {
	return new ArrayBuffer(length + 2);
}

var result = ArrayBuffer.fromBigInt.call(Ctor, 0x0102n);
assert.sameValue(result.byteLength, 4, 'result.byteLength');
assert.compareArray(Array.from(new Uint8Array(result)), [1, 2, 0, 0]);
