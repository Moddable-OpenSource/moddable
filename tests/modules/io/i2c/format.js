/*---
description: 
flags: [onlyStrict]
---*/

if (!(device?.I2C?.default) || !($TESTMC?.config?.i2c))
    throw new Error("Default I2C device and test harness i2c configuration must be provided")

const I2C = device.I2C.default.io;
const defaultConfiguration = {...device.I2C.default, ...$TESTMC.config.i2c};

let i2c = new I2C(defaultConfiguration);

// Invalid formats

const invalidFormats = [
	null,
	undefined,
	123,
    "number",
	"object",
	"invalid",
	"socket/tcp",
	"string;ascii",
	"string;utf8"
]

for (let invalidFormat of invalidFormats) {
	assert.throws(RangeError, () => {
        i2c.format = invalidFormat;
	}, `I2C constructor should throw an error when invalid format "${invalidFormat}" is specified`);
}

// Valid format

i2c.format = "buffer";

i2c.close();