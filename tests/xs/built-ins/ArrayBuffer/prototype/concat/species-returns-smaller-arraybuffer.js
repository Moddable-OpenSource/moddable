/*---
description: concat throws TypeError when the species constructor returns an ArrayBuffer smaller than requested
features: [Symbol.species]
---*/

var source = new ArrayBuffer(4);
source.constructor = {
	[Symbol.species]: function(length) {
		return new ArrayBuffer(length - 1);
	}
};

assert.throws(TypeError, function() {
	source.concat(new ArrayBuffer(4));
});
