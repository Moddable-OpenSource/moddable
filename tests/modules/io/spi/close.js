/*---
description: 
flags: [onlyStrict]
---*/

const SPI = device.io.SPI;

let spi = new SPI({
	...device.SPI.default,
	hz: 10_000_000
});

assert.throws(SyntaxError, () => spi.close.call(new $TESTMC.HostObjectChunk), "close invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => spi.close.call(new $TESTMC.HostObject), "close invalid this - HostObject");
assert.throws(SyntaxError, () => spi.close.call(new $TESTMC.HostBuffer), "close invalid this - HostBuffer");

spi.close();

assert.throws(SyntaxError, () => spi.write(new ArrayBuffer(16)), "write unavailable after close");

spi.close();		// mutliple close is allowed
