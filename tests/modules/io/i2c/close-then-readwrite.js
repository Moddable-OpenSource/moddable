/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.I2C?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

const I2C = device.I2C.default.io;
const defaultConfiguration = {...device.I2C.default, ...$TESTMC.config.i2c};

let i2c = new I2C(defaultConfiguration);

i2c.close();

assert.throws(SyntaxError, () => {
    i2c.read(1);
}, `I2C.read should throw when called after instance is closed`);

i2c = new I2C(defaultConfiguration);

i2c.close();

assert.throws(SyntaxError, () => {
    i2c.write(new Uint8Array([1,2,3]));
}, `I2C.write should throw when called after instance is closed`);