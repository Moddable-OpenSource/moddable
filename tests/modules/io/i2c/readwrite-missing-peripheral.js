/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.I2C?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

if (!($TESTMC?.config.i2c.unusedAddress))
    throw new Error("Test harness must provide an unused I2C address")

const I2C = device.I2C.default.io;
const defaultConfiguration = {...device.I2C.default, ...$TESTMC.config.i2c};

let i2c = new I2C({
    ...defaultConfiguration,
    address: $TESTMC.config.i2c.unusedAddress
});

assert.throws(Error, () => {
    i2c.write(new Uint8Array([1,2,3]));
}, `I2C.write should throw when peripheral is not present at address`);

i2c.close(); 

i2c = new I2C({
    ...defaultConfiguration,
    address: $TESTMC.config.i2c.unusedAddress
});

assert.throws(Error, () => {
    i2c.read(1);
}, `I2C.read should throw when peripheral is not present at address`);

i2c.close();
