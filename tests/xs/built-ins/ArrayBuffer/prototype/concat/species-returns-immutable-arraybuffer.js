/*---
description: concat throws TypeError when the species constructor returns an immutable ArrayBuffer
features: [immutable-arraybuffer, Symbol.species]
---*/

var source = new Uint8Array([1, 2]).buffer;
source.constructor = {
	[Symbol.species]: function(length) {
		return new ArrayBuffer(length).transferToImmutable();
	}
};

assert.throws(TypeError, function() {
	source.concat(new ArrayBuffer(2));
});
