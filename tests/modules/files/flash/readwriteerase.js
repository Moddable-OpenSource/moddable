/*---
description: 
flags: [module]
---*/

// N.B. This test assumes NAND flash (erase to 1, write 0). It is the only flash test with that assumption.

import Flash from "flash";

const f = new Flash($TESTMC.config.flashParition);
const {byteLength, blockSize} = f;

f.erase(0);
f.erase(1);
f.erase(2);
assert(check(0 * blockSize, blockSize, 0xff), "erase block 0");
assert(check(1 * blockSize, blockSize, 0xff), "erase block 1");
assert(check(2 * blockSize, blockSize, 0xff), "erase block 2");

fill(blockSize >> 1, blockSize, 0xfe);
assert(check(blockSize >> 1, blockSize, 0xfe), "clear bit 0 across blocks");

fill(blockSize >> 1, blockSize, 0x7f);
assert(check(blockSize >> 1, blockSize, 0x7e), "clear bit 7 across blocks");

f.erase(1);
assert(check(blockSize >> 1, blockSize >> 1, 0x7e), "partial erase - bottom half");
assert(check(blockSize, blockSize >> 1, 0xff), "partial erase - top half");

fill(blockSize >> 1, blockSize, 0);
assert(check(blockSize >> 1, blockSize, 0), "write zero across blocks");

for (let i = 0, byte = Uint8Array.of(0xa5); i < blockSize; i++)
	f.write(blockSize * 2 + i, 1, byte);
assert(check(blockSize * 2, blockSize, 0xa5), "fill block a byte at a time with 0xA5");

const buffers = [];
buffers.push(new Uint8Array(new ArrayBuffer(5), 3, 1));
buffers.at(-1)[0] = 0x3C;
buffers.push(new DataView(new SharedArrayBuffer(10), 8, 1));
buffers.at(-1).setUint8(0, 0x3C);
buffers.push(new ArrayBuffer(2));
(new Uint8Array(buffers.at(-1)))[0] = 0x3C;
buffers.push(new SharedArrayBuffer(4));
(new Uint8Array(buffers.at(-1)))[0] = 0x3C;
buffers.push(new $TESTMC.HostBuffer(1));
(new Uint8Array(buffers.at(-1)))[0] = 0x3C;

f.erase(2);
for (let i = 0; i < blockSize; i++)
	f.write((blockSize * 2) + i, 1, buffers[i % buffers.length]);
assert(check(blockSize * 2, blockSize, 0x3C), "fill block byte at a time from different kind of buffers");




function fill(offset, byteLength, value) {
	const b = new Uint8Array(byteLength);
	b.fill(value);
	f.write(offset, byteLength, b);
}

function check(offset, byteLength, value) {
	const b = new Uint8Array(f.read(offset, byteLength));
	for (let i = 0; i < byteLength; i++) {
		const t = b[i];
		if (b[i] !== value)
			return false
	}
	return true;
}
