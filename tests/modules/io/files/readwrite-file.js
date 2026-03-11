/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "testfile.dat";

files.delete(path);
assert(!files.status(path).isFile());

let f = files.openFile({path, mode: "w+"});
assert.throws(SyntaxError, () => f.read());
assert.throws(SyntaxError, () => f.read(10));
assert.throws(SyntaxError, () => f.write());
assert.throws(SyntaxError, () => f.write(new ArrayBuffer(10)));

let bytes = new Uint8Array(256);
for (let i = 0; i <= bytes.length; i++) 
	bytes[i] = 255 - i;

f.write(bytes, 256);

bytes.fill(0xff)
assert.sameValue(f.read(bytes, 0), 256);
for (let i = 0; i < 256; i++)
	assert.sameValue(bytes[i], 0);
assert.sameValue(f.read(bytes, 256), 256);
for (let i = 0; i < 256; i++)
	assert.sameValue(bytes[i], 255 - i);

bytes = new Uint8Array(f.read(256, 0));
for (let i = 0; i < 256; i++)
	assert.sameValue(bytes[i], 0);
bytes = new Uint8Array(f.read(256, 256));
for (let i = 0; i < 256; i++)
	assert.sameValue(bytes[i], 255 - i);

f.close();

assert.throws(SyntaxError, () => f.read(5, 1));
assert.throws(SyntaxError, () => f.write(bytes, 0));
