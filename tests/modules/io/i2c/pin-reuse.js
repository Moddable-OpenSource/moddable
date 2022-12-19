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

// Reusing data and clock with a different address is allowed

i2c_two = new I2C({
    ...defaultConfiguration,
    address: $TESTMC.config.i2c.unusedAddress
});

i2c.close();
i2c_two.close();
