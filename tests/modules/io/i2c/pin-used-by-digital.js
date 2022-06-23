/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.I2C?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

const I2C = device.I2C.default.io;
const Digital = device.io.Digital;
const defaultConfiguration = {...device.I2C.default, ...$TESTMC.config.i2c};

let i2c, digital;

digital = new Digital({
    pin: defaultConfiguration.data,
    mode: Digital.Output
});

assert.throws(RangeError, () => {
    i2c = new I2C(defaultConfiguration);
}, `I2C constructor should throw an error when data pin is already in use`);

digital.close();

digital = new Digital({
    pin: defaultConfiguration.clock,
    mode: Digital.Output
});

assert.throws(RangeError, () => {
    i2c = new I2C(defaultConfiguration);
}, `I2C constructor should throw an error when clock pin is already in use`);

// Pin reuse is allowed after instances are closed 

digital.close();

i2c = new I2C(defaultConfiguration);

i2c.close();