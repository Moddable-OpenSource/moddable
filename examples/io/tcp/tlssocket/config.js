import device from "embedded:provider/builtin";
import TLS from "embedded:io/socket/tcp/tls";

device.io ??= {};
device.io.TLS = TLS;
