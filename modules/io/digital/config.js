import device from "embedded:provider/builtin";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";

device.io ??= {};
device.io.Digital = Digital;
device.io.DigitalBank = DigitalBank;
