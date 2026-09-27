/*---
description: slice throws TypeError when the species constructor returns an immutable ArrayBuffer
features: [immutable-arraybuffer, Symbol.species]
---*/

var source = new ArrayBuffer(8);
source.constructor = {
	[Symbol.species]: function(length) {
		return new ArrayBuffer(length).transferToImmutable();
	}
};

assert.throws(TypeError, function() {
	source.slice(0, 4);
});
