import device from "embedded:provider/builtin";
import PulseWidth from "embedded:io/pulsewidth";

device.io ??= {};
device.io.PulseWidth = PulseWidth;
