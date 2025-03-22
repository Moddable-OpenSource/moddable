/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
const blockLength = f.status().blockLength;

f.write(new ArrayBuffer(blockLength), 0);
assertValue(f.read(blockLength, 0), 0);

f.eraseBlock(0);
assertValue(f.read(blockLength, 0), 0xff);

f.write((new Uint8Array(blockLength / 2)).fill(0x80), blockLength / 2);
assertValue(f.read(blockLength / 2, 0), 0xff);
assertValue(f.read(blockLength / 2, blockLength / 2), 0x80);

f.eraseBlock(0, 2);
f.write(Uint8Array.of(0xfe, 0xfc), blockLength - 1);

assertValue(f.read(blockLength - 1, 0), 0xff);
assertValue(f.read(1, blockLength - 1), 0xfe);
assertValue(f.read(1, blockLength), 0xfc);
assertValue(f.read(blockLength - 1, blockLength + 1), 0xff);

f.write(Uint8Array.of(0xf0, 0xe0), blockLength - 1);
assertValue(f.read(1, blockLength - 1), 0xf0);
assertValue(f.read(1, blockLength), 0xe0);

let b = new Uint8Array(2);
assert.sameValue(b.length, f.read(b, blockLength - 1));
assert.sameValue(b[0], 0xf0);
assert.sameValue(b[1], 0xe0);

b = new Uint8Array(blockLength - 1);
assert.sameValue(b.length, f.read(b, 0));
assertValue(b, 0xff);

f.close();

function assertValue(buffer, value)
{
	if (buffer instanceof ArrayBuffer)
		buffer = new Uint8Array(buffer);

	for (let i = 0; i < buffer.length; i++)
		assert.sameValue(value, buffer[i]);
}
