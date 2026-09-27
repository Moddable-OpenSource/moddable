/*---
description: concat throws TypeError when the species constructor returns this
features: [Symbol.species]
---*/

var source = new ArrayBuffer(4);
source.constructor = {
	[Symbol.species]: function() {
		return source;
	}
};

assert.throws(TypeError, function() {
	source.concat();
});
