/*---
description: 
flags: [onlyStrict]
---*/

const SPI = device.io.SPI;

const options = {
	...device.SPI.default,
	hz: 10_000_000
};

let spi = new SPI({
	...options,
	format: "buffer"
});
spi.close();

spi = new SPI({...options});
assert.sameValue("buffer", spi.format, `default format"`);
spi.close();

assert.throws(RangeError, () => new SPI({...options, format: "number"}), "invalid format - number"); 
assert.throws(RangeError, () => new SPI({...options, format: "nonsense"}), "invalid format - nonsense"); 

spi = new SPI({...options, format: "buffer"});
assert.sameValue("buffer", spi.format, `format = "buffer"`);

assert.throws(RangeError, () => spi.format = "number", "set invalid format - number"); 
assert.sameValue("buffer", spi.format, `format = "buffer"`);

spi.format = "buffer";
assert.sameValue("buffer", spi.format, `format = "buffer"`);
	
spi.close();
