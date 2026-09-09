import device from "embedded:provider/builtin";
import Analog from "embedded:io/analog";

device.io ??= {};
device.io.Analog = Analog;
