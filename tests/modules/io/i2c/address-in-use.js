/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.I2C?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

const I2C = device.I2C.default.io;
const defaultConfiguration = {...device.I2C.default, ...$TESTMC.config.i2c};

let i2c, i2c_two;

i2c = new I2C(defaultConfiguration);

// Reusing an address is not allowed

assert.throws(RangeError, () => {
	i2c_two = new I2C(defaultConfiguration);
}, `I2C constructor should throw an error when address is already in use`);

// Address can be used again after close

i2c.close();

i2c_two = new I2C(defaultConfiguration);

i2c_two.close();
