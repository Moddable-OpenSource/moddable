import builtin from "embedded:provider/builtin";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.Digital = Digital;
device.io.DigitalBank = DigitalBank;
