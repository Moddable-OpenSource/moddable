import device from "embedded:provider/builtin";
import PulseCount from "embedded:io/pulsecount";

device.io ??= {};
device.io.PulseCount = PulseCount;
