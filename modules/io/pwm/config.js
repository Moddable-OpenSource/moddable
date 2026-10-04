import builtin from "embedded:provider/builtin";
import PWM from "embedded:io/pwm";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.PWM = PWM;
