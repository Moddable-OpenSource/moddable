/*---
description: fromBigInt throws TypeError when this constructs a detached ArrayBuffer
features: [BigInt, arraybuffer-transfer]
---*/

function Ctor(length) {
	var result = new ArrayBuffer(length);
	result.transfer();
	return result;
}

assert.throws(TypeError, function() {
	ArrayBuffer.fromBigInt.call(Ctor, 0x0102n);
});
