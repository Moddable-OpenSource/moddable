import builtin from "embedded:provider/builtin";
import flash from "embedded:storage/flash";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.flash = flash;
