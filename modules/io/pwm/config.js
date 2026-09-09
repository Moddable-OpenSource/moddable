import device from "embedded:provider/builtin";
import PWM from "embedded:io/pwm";

device.io ??= {};
device.io.PWM = PWM;
