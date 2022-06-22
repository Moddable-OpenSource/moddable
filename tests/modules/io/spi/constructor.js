/*---
description: 
flags: [onlyStrict]
---*/

const SPI = device.io.SPI;
const Digital = device.io.Digital;

const options = Object.freeze({
	...device.SPI.default,
	hz: 10_000_000
});

assert.throws(SyntaxError, () => new SPI, "no options"); 
assert.throws(Error, () => new SPI(1), "invalid options - number"); 
assert.throws(Error, () => new SPI("one"), "invalid options - string"); 
assert.throws(Error, () => new SPI("1n"), "invalid options - BigInt"); 
assert.throws(Error, () => new SPI({}), "invalid options - empty"); 

let spi = new SPI(options);
let o;

assert.throws(Error, () => new SPI(options), "in use"); 
spi.close();

spi = new SPI(options);		// can allocate after close
spi.close();

spi = new SPI({...options, clock: options.clock.toString()});
spi.close();

/*
	required pins
*/
o = {...options};
delete o.clock;
assert.throws(Error, () => new SPI(o), "no clock"); 

o = {...options};
delete o.in;
delete o.out;
assert.throws(Error, () => new SPI(o), "in or out required"); 

o = {...options};
o.in = o.out;
spi = new SPI(o);		// in & out may be the same (3-pin SPI)
spi.close(); 

o = {...options};
o.out = undefined;
assert.throws(Error, () => new SPI(o), "out cannot be undefined "); 

o = {...options};
o.in = undefined;
assert.throws(Error, () => new SPI(o), "in cannot be undefined "); 

o = {...options};
o.clock = undefined;
assert.throws(Error, () => new SPI(o), "clock cannot be undefined "); 

/*
	select
*/
let digital = new Digital({pin: $TESTMC.config.spi.select, mode: Digital.Output});
assert.throws(Error, () => new SPI({...options, select: $TESTMC.config.spi.select}), "select in use");
digital.close(); 

spi = new SPI({...options, select: $TESTMC.config.spi.select});
spi.close();

spi = new SPI({...options, select: $TESTMC.config.spi.select.toString()});
spi.close();

assert.throws(TypeError, () => new SPI({...options, select: BigInt($TESTMC.config.spi.select)}), "select bigint");

/*
	hz
*/
$TESTMC.config.spi.hzs.forEach(hz => {
	spi = new SPI({...options, hz});
	spi.close();
});

o = {...options};
delete o.hz;
assert.throws(RangeError, () => new SPI(o), "no hz"); 

assert.throws(RangeError, () => new SPI({...options, hz: 0}));
assert.throws(RangeError, () => new SPI({...options, hz: -1}));
assert.throws(RangeError, () => new SPI({...options, hz: 1_000_000_000}));
assert.throws(TypeError, () => new SPI({...options, hz: 100n}));
assert.throws(TypeError, () => new SPI({...options, hz: Symbol()}));

/*
	active
*/
spi = new SPI({...options, active: 0, select: $TESTMC.config.spi.select});
spi.close();

spi = new SPI({...options, active: 1, select: $TESTMC.config.spi.select});
spi.close();

assert.throws(RangeError, () => new SPI({...options, active: -1, select: $TESTMC.config.spi.select}));
assert.throws(RangeError, () => new SPI({...options, active: 2, select: $TESTMC.config.spi.select}));
assert.throws(TypeError, () => new SPI({...options, active: 1n, select: $TESTMC.config.spi.select}));

/*
	mode
*/
for (let mode = 0; mode < 4; mode += 1) {
	spi = new SPI({...options, mode});
	spi.close();
}

assert.throws(RangeError, () => new SPI({...options, mode: -1}));
assert.throws(RangeError, () => new SPI({...options, mode: 500}));
assert.throws(TypeError, () => new SPI({...options, mode: 1n}));

/*
	port
*/
$TESTMC.config.spi.ports.forEach(port => {
	spi = new SPI({...options, port});
	spi.close();
});

$TESTMC.config.spi.invalidPorts.forEach(port => {
	assert.throws(RangeError, () => new SPI({...options, port}), "invalid port");
});

/*
// this should succceed but SPI currently only allows once instance to use a bus at a time
spi = new SPI({...options, port: $TESTMC.config.spi.ports[0]});
let ts = new SPI({...options, port: $TESTMC.config.spi.ports[0]});
spi.close();
ts.close();
*/
