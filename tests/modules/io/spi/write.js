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

assert.throws(SyntaxError, () => spi.write.call(new $TESTMC.HostObjectChunk, buffer), "write invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => spi.write.call(new $TESTMC.HostObject, buffer), "write invalid this - HostObject");
assert.throws(SyntaxError, () => spi.write.call(new $TESTMC.HostBuffer, buffer), "write invalid this - HostBuffer");

assert.throws(SyntaxError, () => spi.write(), "write - no argument");
assert.throws(TypeError, () => spi.write(undefined), "write - undefined argument");
assert.throws(TypeError, () => spi.write({}), "write - object argument");
assert.throws(TypeError, () => spi.write(0), "write - 0 bytes");
assert.throws(TypeError, () => spi.write(-1), "write - -1 bytes");
assert.throws(TypeError, () => spi.write(new Uint32Array(buffer)), "write - Uint32Array");

spi.write(buffer);
spi.write(new SharedArrayBuffer(256));
spi.write(new Int8Array(buffer));
spi.write(new Int8Array(buffer, 16, 16));
spi.write(new Uint8Array(buffer, 17, 17));
spi.write(new Uint8Array(buffer, 18, 18));
spi.write(new Uint8Array(buffer, 19, 1));
spi.write(new Uint8Array(buffer, 19, 2));
spi.write(new Uint8Array(buffer, 19, 3));

spi.close();
