import builtin from "embedded:provider/builtin";
import Analog from "embedded:io/analog";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.Analog = Analog;
