import builtin from "embedded:provider/builtin";
import I2C from "embedded:io/i2c";
import SMBus from "embedded:io/smbus";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.I2C = I2C;
device.io.SMBus = SMBus;
