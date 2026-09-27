/*---
description: transfer checks for a detached source before checking the new length against maxByteLength
features: [resizable-arraybuffer, arraybuffer-transfer]
---*/

var source = new ArrayBuffer(8, { maxByteLength: 16 });
source.transfer();

assert.throws(TypeError, function() {
	source.transfer(32);
});
