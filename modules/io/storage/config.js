import builtin from "embedded:provider/builtin";
import keyValue from "embedded:storage/key-value";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.keyValue = keyValue;
