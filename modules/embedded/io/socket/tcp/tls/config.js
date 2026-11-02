import builtin from "embedded:provider/builtin";
import TLS from "embedded:io/socket/tcp/tls";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.TLS = TLS;
