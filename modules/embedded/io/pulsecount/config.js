import builtin from "embedded:provider/builtin";
import PulseCount from "embedded:io/pulsecount";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.PulseCount = PulseCount;
