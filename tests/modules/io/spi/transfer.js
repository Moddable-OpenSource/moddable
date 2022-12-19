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

assert.throws(SyntaxError, () => spi.transfer.call(new $TESTMC.HostObjectChunk, buffer), "transfer invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => spi.transfer.call(new $TESTMC.HostObject, buffer), "transfer invalid this - HostObject");
assert.throws(SyntaxError, () => spi.transfer.call(new $TESTMC.HostBuffer, buffer), "transfer invalid this - HostBuffer");

assert.throws(SyntaxError, () => spi.transfer(), "transfer - no argument");
assert.throws(TypeError, () => spi.transfer(undefined), "transfer - undefined argument");
assert.throws(TypeError, () => spi.transfer({}), "transfer - object argument");
assert.throws(TypeError, () => spi.transfer(0), "transfer - number");

spi.transfer(buffer);
spi.transfer(new SharedArrayBuffer(256));
spi.transfer(new Uint8Array(buffer));
spi.transfer(new Int8Array(buffer));
assert.throws(TypeError, () => spi.transfer(new Uint32Array(buffer)), "transfer - Uint32Array");

// transfer full buffer
buffer = new Uint8Array(buffer);
for (let i = 0; i < 256; i++)
	buffer[i] = i;
spi.transfer(buffer);
let changed = false;
for (let i = 0; i < 256; i++)
	changed = changed || (buffer[i] !== i);
if (!changed)
	throw new Error("transfer didn't change buffer");

// transfer into middle of buffer
for (let i = 0; i < 256; i++)
	buffer[i] = i;
spi.transfer(new Uint8Array(buffer.buffer, 16, 16));
for (let i = 0; i < 16; i++)
	assert.sameValue(i, buffer[i], "start should be unchanged");
changed = false;
for (let i = 16; i < 32; i++)
	changed = changed || (buffer[i] !== i);
if (!changed)
	throw new Error("transfer didn't change middle of buffer");
for (let i = 32; i < 256; i++)
	assert.sameValue(i, buffer[i], "end should be unchanged");

spi.close();
