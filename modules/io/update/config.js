import builtin from "embedded:provider/builtin";
import update from "embedded:update";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.update = update;
