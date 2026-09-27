/*---
description: fromBigInt throws TypeError when this constructs an ArrayBuffer smaller than requested
features: [BigInt]
---*/

function Ctor(length) {
	return new ArrayBuffer(length - 1);
}

assert.throws(TypeError, function() {
	ArrayBuffer.fromBigInt.call(Ctor, 0x010203n, 8);
});
