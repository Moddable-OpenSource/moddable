import device from "embedded:provider/builtin";
import SPI from "embedded:io/spi";

device.io ??= {};
device.io.SPI = SPI;
