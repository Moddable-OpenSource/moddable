/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.i2c?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

const I2C = device.i2c.default.io;
const defaultConfiguration = {...device.i2c.default, ...$TESTMC.config.i2c};

let i2c = new I2C(defaultConfiguration);

i2c.close();
i2c.close();