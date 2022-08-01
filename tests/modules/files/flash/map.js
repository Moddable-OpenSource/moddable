/*---
description: 
flags: [module]
---*/

import Flash from "flash";

let f = new Flash($TESTMC.config.flashParition);
const {byteLength, blockSize} = f;

const firstBlock = 16, blockCount = 4;
for (let i = 0; i < blockCount; i++) {
	let block = new Uint8Array(blockSize);
	block.fill(i);

	f.erase(firstBlock + i);
	f.write((firstBlock + i) * blockSize, blockSize, block);
}

assert.throws(SyntaxError, () => f.map.call(new $TESTMC.HostObject, 0, 64), "map with non-flash this");

f.map();
f.map();
f.map();
const map = f.map();

assert.sameValue(typeof map.byteLength, "number", "map byteLength is number");
assert.throws(TypeError, () => map.byteLength = blockSize, "map byteLength is read-only");
assert.sameValue(map.byteLength, byteLength, "map byteLength matches partition byteLength");

const bytes = new Uint8Array(map);
for (let i = 0; i < blockCount * blockSize; i++)
	assert.sameValue(bytes[i + (firstBlock * blockSize)], (i / blockSize) | 0, "map contents incorrect");

assert.throws(TypeError, () => bytes[0] = 255, "map contents are read-only");
assert.sameValue(undefined, bytes[map.byteLength], "cannot read bytes outside map");
assert.sameValue(undefined, bytes[map.byteLength + 1], "cannot read bytes outside map");

f = undefined;
$262.gc();
let b = bytes[0];
