/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.I2C?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

const I2C = device.I2C.default.io;
const defaultConfiguration = {...device.I2C.default, ...$TESTMC.config.i2c};

const {data, clock, hz, address, port = undefined} = defaultConfiguration;

let i2c;

// Required options object properties

assert.throws(RangeError, () => {
	i2c = new I2C({
	   clock,
	   hz,
	   address,
	   ...(port !== undefined ? {port} : {})
	});
}, `I2C constructor should throw an error when no data property is specified`);

assert.throws(RangeError, () => {
	i2c = new I2C({
	   data,
	   hz,
	   address,
	   ...(port !== undefined ? {port} : {})
	});
}, `I2C constructor should throw an error when no clock property is specified`);

assert.throws(RangeError, () => {
	i2c = new I2C({
		data,
		clock,
		address,
		...(port !== undefined ? {port} : {})
	});
}, `I2C constructor should throw an error when no hz property is specified`);

assert.throws(RangeError, () => {
	i2c = new I2C({
		data,
		clock,
		hz,
		...(port !== undefined ? {port} : {})
	});
}, `I2C constructor should throw an error when no address property is specified`);

// Invalid pin specifiers

const errorPins = [undefined];

if (!$TESTMC.config.invalidPins)
	throw new Error("Test harness must provide invalid pin specifiers to test")

for (let invalidPin of $TESTMC.config.invalidPins) {
	assert.throws(RangeError, () => {
		i2c = new I2C({
			...defaultConfiguration,
			data: invalidPin
		});
	}, `I2C constructor should throw an error when invalid data pin is specified`);
	assert.throws(RangeError, () => {
		i2c = new I2C({
			...defaultConfiguration,
			clock: invalidPin
		});
	}, `I2C constructor should throw an error when invalid clock pin is specified`);
}
for (let invalidPin of errorPins) {
	assert.throws(Error, () => {
		i2c = new I2C({
			...defaultConfiguration,
			data: invalidPin
		});
	}, `I2C constructor should throw an error when invalid data pin is specified`);
	assert.throws(Error, () => {
		i2c = new I2C({
			...defaultConfiguration,
			clock: invalidPin
		});
	}, `I2C constructor should throw an error when invalid clock pin is specified`);
}

// Invalid hz

let invalidHzs = [
	0,
	-1,
	NaN,
	undefined,
	null,
	{test: "object"},
	"string"
]

for (let invalidHz of invalidHzs) {
	assert.throws(RangeError, () => {
		i2c = new I2C({
			...defaultConfiguration,
			hz: invalidHz
		})
	}, `I2C constructor should throw an error when invalid hz is specified`);
}

// Invalid address

let invalidAddresses = [
	128,
	-1
]

// @@ addresses that I think should be invalid but are not throwing errors: NaN, null

for (let invalidAddress of invalidAddresses) {
	assert.throws(RangeError, () => {
		i2c = new I2C({
			...defaultConfiguration,
			address: invalidAddress
		})
	}, `I2C constructor should throw an error when invalid address is specified`)
}

invalidAddresses = [
	undefined
]

for (let invalidAddress of invalidAddresses) {
	assert.throws(Error, () => {
		i2c = new I2C({
			...defaultConfiguration,
			address: invalidAddress
		})
	}, `I2C constructor should throw an error when invalid address is specified`)
}

// Invalid format

const invalidFormats = [
	"number",
	"object",
	"invalid",
	"socket/tcp",
	"string;ascii",
	"string;utf8"
]

for (let invalidFormat of invalidFormats) {
	assert.throws(RangeError, () => {
		i2c = new I2C({
			...defaultConfiguration,
			format: invalidFormat
		})
	}, `I2C constructor should throw an error when invalid format "${invalidFormat}" is specified`);
}
