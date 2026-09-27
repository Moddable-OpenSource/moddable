/*---
description: transfer copies from the length of the source after converting newLength
includes: [compareArray.js]
features: [resizable-arraybuffer, arraybuffer-transfer]
---*/

var source = new ArrayBuffer(64, { maxByteLength: 64 });
new Uint8Array(source).fill(0xFF);

var dest = source.transfer({
	valueOf() {
		source.resize(1);
		$262.gc();		// reallocate the chunk at the smaller size
		return 64;
	}
});

var expected = new Array(64).fill(0);
expected[0] = 0xFF;

assert.sameValue(source.detached, true, 'source.detached');
assert.sameValue(dest.byteLength, 64, 'dest.byteLength');
assert.compareArray(Array.from(new Uint8Array(dest)), expected);
