/*---
description: concat copies what fits when constructing the result grows this
includes: [compareArray.js]
features: [resizable-arraybuffer]
---*/

var source = new ArrayBuffer(4, { maxByteLength: 1024 });
new Uint8Array(source).set([1, 2, 3, 4]);
var other = new Uint8Array([5, 6]).buffer;

Object.defineProperty(source, "constructor", {
	get() {
		source.resize(1024);
		new Uint8Array(source).fill(9, 4);
		return ArrayBuffer;
	}
});

var dest = source.concat(other);
assert.sameValue(dest.byteLength, 6, 'dest.byteLength');
assert.compareArray(Array.from(new Uint8Array(dest)), [1, 2, 3, 4, 9, 9]);
