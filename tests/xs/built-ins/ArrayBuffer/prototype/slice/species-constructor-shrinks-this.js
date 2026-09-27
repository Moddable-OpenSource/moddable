/*---
description: slice copies what fits when constructing the result shrinks this
includes: [compareArray.js]
features: [resizable-arraybuffer, Symbol.species]
---*/

var source = new ArrayBuffer(8, { maxByteLength: 16 });
var sourceArray = new Uint8Array(source);
for (var i = 0; i < 8; i++)
	sourceArray[i] = i + 1;

source.constructor = {
	[Symbol.species]: function(length) {
		source.resize(4);
		return new ArrayBuffer(length);
	}
};

var dest = source.slice(2, 8);
assert.sameValue(dest.byteLength, 6, 'dest.byteLength');
assert.compareArray(Array.from(new Uint8Array(dest)), [3, 4, 0, 0, 0, 0]);

source.resize(8);
dest = source.slice(6, 8);
assert.compareArray(Array.from(new Uint8Array(dest)), [0, 0], 'start beyond shrunk length');
