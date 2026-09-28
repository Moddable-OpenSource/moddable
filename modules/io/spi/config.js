import builtin from "embedded:provider/builtin";
import SPI from "embedded:io/spi";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.SPI = SPI;
