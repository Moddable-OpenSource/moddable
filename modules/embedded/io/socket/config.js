import builtin from "embedded:provider/builtin";
import TCP from "embedded:io/socket/tcp";
import UDP from "embedded:io/socket/udp";
import Listener from "embedded:io/socket/listener";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

device.io ??= {};
device.io.TCP = TCP;
device.io.UDP = UDP;
device.io.Listener = Listener;
