/*---
description: transferToFixedLength is not limited by the source maxByteLength
features: [resizable-arraybuffer, arraybuffer-transfer]
---*/

var source = new ArrayBuffer(8, { maxByteLength: 16 });
var dest = source.transferToFixedLength(32);

assert.sameValue(source.detached, true, 'source.detached');
assert.sameValue(dest.byteLength, 32, 'dest.byteLength');
assert.sameValue(dest.resizable, false, 'dest.resizable');
