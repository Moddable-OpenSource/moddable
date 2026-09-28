import builtin from "embedded:provider/builtin";
import Serial from "embedded:io/serial";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.Serial = Serial;
