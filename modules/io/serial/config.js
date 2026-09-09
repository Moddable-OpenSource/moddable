import device from "embedded:provider/builtin";
import Serial from "embedded:io/serial";

device.io ??= {};
device.io.Serial = Serial;
