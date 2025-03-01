/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
const {blockLength, blocks} = f.status();

f.write(new ArrayBuffer(blockLength), 0);
f.write(new ArrayBuffer(blockLength), (blocks - 1) * blockLength);
assertValue(f.read(blockLength, 0), 0);
assertValue(f.read(blockLength, (blocks - 1) * blockLength), 0);

f.eraseBlock(0);
f.eraseBlock(blocks - 1);

assertValue(f.read(blockLength, 0), 0xff);
assertValue(f.read(blockLength, (blocks - 1) * blockLength), 0xff);

f.write(new ArrayBuffer(blockLength * 2), blockLength);
assertValue(f.read(blockLength * 2, blockLength), 0);
f.eraseBlock("1", "3");
assertValue(f.read(blockLength * 2, blockLength), 0xff);

assert.throws(SyntaxError, () => f.eraseBlock());
assert.throws(TypeError, () => f.eraseBlock(1n));
assert.throws(TypeError, () => f.eraseBlock(0, 1n));

f.close();

function assertValue(buffer, value)
{
	if (buffer instanceof ArrayBuffer)
		buffer = new Uint8Array(buffer);

	for (let i = 0; i < buffer.length; i++)
		assert.sameValue(value, buffer[i]);
}
