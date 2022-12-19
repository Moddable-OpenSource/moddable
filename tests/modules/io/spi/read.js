/*---
description: 
flags: [onlyStrict]
---*/

const SPI = device.io.SPI;

let spi = new SPI({
	...device.SPI.default,
	hz: 10_000_000
});

let buffer = new ArrayBuffer(256);

assert.throws(SyntaxError, () => spi.read.call(new $TESTMC.HostObjectChunk, buffer), "read invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => spi.read.call(new $TESTMC.HostObject, buffer), "read invalid this - HostObject");
assert.throws(SyntaxError, () => spi.read.call(new $TESTMC.HostBuffer, buffer), "read invalid this - HostBuffer");

assert.throws(SyntaxError, () => spi.read(), "read - no argument");
assert.throws(RangeError, () => spi.read(undefined), "read - undefined argument");
assert.throws(TypeError, () => spi.read({}), "read - object argument");
assert.throws(RangeError, () => spi.read(0), "read - 0 bytes");
assert.throws(RangeError, () => spi.read(-1), "read - -1 bytes");

let b = spi.read(256);
assert.sameValue(256, b.byteLength, `b.byteLength = 256`);
b = spi.read(99);
assert.sameValue(99, b.byteLength, `b.byteLength = 99`);
b = spi.read(1);
assert.sameValue(1, b.byteLength, `b.byteLength = 1`);

spi.read(buffer);
spi.read(new SharedArrayBuffer(256));
spi.read(new Uint8Array(buffer));
spi.read(new Int8Array(buffer));
assert.throws(TypeError, () => spi.read(new Uint32Array(buffer)), "read - Uint32Array");

// read full buffer
buffer = new Uint8Array(buffer);
for (let i = 0; i < 256; i++)
	buffer[i] = i;
spi.read(buffer);
let changed = false;
for (let i = 0; i < 256; i++)
	changed = changed || (buffer[i] !== i);
if (!changed)
	throw new Error("read didn't change buffer");

// read into middle of buffer
for (let i = 0; i < 256; i++)
	buffer[i] = i;
spi.read(new Uint8Array(buffer.buffer, 16, 16));
for (let i = 0; i < 16; i++)
	assert.sameValue(i, buffer[i], "start should be unchanged");
changed = false;
for (let i = 16; i < 32; i++)
	changed = changed || (buffer[i] !== i);
if (!changed)
	throw new Error("read didn't change middle of buffer");
for (let i = 32; i < 256; i++)
	assert.sameValue(i, buffer[i], "end should be unchanged");

spi.close();
