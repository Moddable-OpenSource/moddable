/*---
description: transfer throws RangeError when the new length exceeds maxByteLength, before detaching the source
features: [resizable-arraybuffer, arraybuffer-transfer]
---*/

var source = new ArrayBuffer(8, { maxByteLength: 16 });

assert.throws(RangeError, function() {
	source.transfer(32);
});
assert.sameValue(source.detached, false, 'source.detached');
assert.sameValue(source.byteLength, 8, 'source.byteLength');

var dest = source.transfer(16);
assert.sameValue(dest.byteLength, 16, 'dest.byteLength');
assert.sameValue(dest.maxByteLength, 16, 'dest.maxByteLength');
