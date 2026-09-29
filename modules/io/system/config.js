import builtin from "embedded:provider/builtin";
import System from "embedded:system";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.system = System;
