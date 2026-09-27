/*---
description: slice throws TypeError when the species constructor returns a detached ArrayBuffer
features: [arraybuffer-transfer, Symbol.species]
---*/

var source = new ArrayBuffer(8);
source.constructor = {
	[Symbol.species]: function(length) {
		var result = new ArrayBuffer(length);
		result.transfer();
		return result;
	}
};

assert.throws(TypeError, function() {
	source.slice(0, 0);
});
