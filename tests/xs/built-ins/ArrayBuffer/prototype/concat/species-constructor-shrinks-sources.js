/*---
description: concat copies what remains when constructing the result shrinks or detaches sources
includes: [compareArray.js]
features: [resizable-arraybuffer, arraybuffer-transfer]
---*/

var source = new ArrayBuffer(4, { maxByteLength: 8 });
new Uint8Array(source).set([1, 2, 3, 4]);
var shrinks = new ArrayBuffer(3, { maxByteLength: 8 });
new Uint8Array(shrinks).set([5, 6, 7]);
var detaches = new Uint8Array([8, 9]).buffer;
var last = new Uint8Array([10]).buffer;

Object.defineProperty(source, "constructor", {
	get() {
		source.resize(2);
		shrinks.resize(1);
		detaches.transfer();
		return ArrayBuffer;
	}
});

var dest = source.concat(shrinks, detaches, last);
assert.sameValue(dest.byteLength, 10, 'dest.byteLength');
assert.compareArray(Array.from(new Uint8Array(dest)), [1, 2, 5, 10, 0, 0, 0, 0, 0, 0]);
