import device from "embedded:provider/builtin";
import TCP from "embedded:io/socket/tcp";
import UDP from "embedded:io/socket/udp";
import Listener from "embedded:io/socket/listener";

device.io ??= {};
device.io.TCP = TCP;
device.io.UDP = UDP;
device.io.Listener = Listener;
