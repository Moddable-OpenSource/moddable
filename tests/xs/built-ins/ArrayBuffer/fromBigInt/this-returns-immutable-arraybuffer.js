/*---
description: fromBigInt throws TypeError when this constructs an immutable ArrayBuffer, leaving it unchanged
features: [BigInt, immutable-arraybuffer]
---*/

var result;
function Ctor(length) {
	result = new ArrayBuffer(length).transferToImmutable();
	return result;
}

assert.throws(TypeError, function() {
	ArrayBuffer.fromBigInt.call(Ctor, 0x0102n);
});
assert.sameValue(result.immutable, true, 'result.immutable');
assert.sameValue(new Uint8Array(result).every(b => b === 0), true, 'immutable result not written');
