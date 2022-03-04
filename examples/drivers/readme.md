# Moddable SDK - Driver Examples

Copyright 2017-2021 Moddable Tech, Inc.<BR>
Revised: November 3, 2021

These examples use various sensor, display, and touch drivers available in the Moddable SDK. This document provides a brief description of each example and components supported. The source code for each driver is located in the [$MODDABLE/modules/drivers](../../modules/drivers) directory.

***

### [`arducamframeserver`](./arducamframeserver)

HTTP server that returns a JPEG image captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/).

***

### [`arducamhttpput`](./arducamhttpput)

HTTP client that posts successive JPEG images captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/) to an HTTP server.

***

### [`arducampreview`](./arducampreview)

Displays a raw 16-bit RGB image captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/).

***

### [`arducamstreamserver`](./arducamstreamserver)

HTTP server that returns a JPEG image captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/) as a multipart HTTP response. The HTTP client can optionally specify the image width and height in the request.

***

### [`BMP180`](./BMP180)

HTTP server that returns a text reading from an I<sup>2</sup>C [Bosch BMP180 Barometric Pressure/Temperature sensor](https://www.adafruit.com/product/1603). The application additionally configures the host as a Wi-Fi base station.

***

### [`bmp280`](./bmp280)

Reads the [BMP280 Temperature/Humidity sensor](https://www.adafruit.com/product/2651).

***

### [`dotstar`](./dotstar)

Progressively renders each row of a color image to an [Adafruit DotStar 144 LED strip](https://www.adafruit.com/product/2242). The [driver](../../modules/drivers/dotstar) communicates with the strip over SPI.

***

### [`ft6206calibrate`](./ft6206calibrate)

Calibration application for FT6206 touch screen on Moddable One and Moddable Two.

***

### [`ga1auv100wp`](./ga1auv100wp)
Reads ambient and UV light measurements from the dual-mode [Sharp UV Light Sensor](http://www.sharp-world.com/products/device/lineup/selection/opto/receiving_light/opic/ultraviolet/index.html). The application configures the sensor operating mode. The [driver](../../modules/drivers/ga1auv100wp) communicates with the sensor over I<sup>2</sup>C using the SMBus protocol.

***

### [`gp2ap01vt00f`](./gp2ap01vt00f)
Reads distance measurements at 100ms intervals from the [Sharp Time-of-Flight Laser Ranging Sensor](http://www.sunnic.com/upfiles/tw_/sharp/GP2AP01VT00F.pdf). The [driver](../../modules/drivers/gp2ap01vt00f) communicates with the sensor over I<sup>2</sup>C.

***

### [`HD44780i-i2c`](./HD44780i-i2c)

Demonstrates use of the the Hitachi HD44780 character LCD module.

***

### [`HMC5883L`](./HMC5883L)

Reads digital compass measurements at 250ms intervals from the I<sup>2</sup>C [SparkFun Triple Axis Magnetometer Breakout - HMC5883L](https://www.sparkfun.com/products/retired/10530).	
***

### [`hx711`](./hx711)

Demonstrates use of the the HX711 Load Cell Amplifier to measure weight.

***

### [`kaluga-buttons`](./kaluga-buttons)

Demonstrates how to use both the touch pad buttons and push buttons on the ESP32‑S2 powered Kaluga board.

> For more information about the Kaluga board, see [this blog post](https://blog.moddable.com/blog/espidf42/).

***

### [`lis3dh`](./lis3dh)

Reads the [Adafruit LIS3DH Triple-Axis Accelerometer](https://www.adafruit.com/product/2809) at 100ms intervals. The [driver](../../modules/drivers/lis3dh) communicates with the accelerometer over I<sup>2</sup>C.

***

### [`lis3dhball`](./lis3dhball)

[Commodetto](../../documentation/commodetto/commodetto.md) app that displays [Adafruit LIS3DH Triple-Axis Accelerometer](https://www.adafruit.com/product/2809) sensor values in real time and animates a bouncing ball based on the X and Y readings.

***

### [`ls013b4dn04`](./ls013b4dn04)

[Commodetto](../../documentation/commodetto/commodetto.md) app for Silicon Labs Gecko devices. Demonstrates use of [this Sharp mirror display](../../documentation/displays/wiring-guide-sharp-memory-1.3-spi.md)  and deep sleep modes.

***

### [`m5atom-echo`](./m5atom-echo)

A simple example to demonstrate built-in hardware features of the M5Atom Echo development board.

***

### [`m5atom-lite`](./m5atom-lite)

A simple example to demonstrate built-in hardware features of the M5Atom Lite development board.

***

### [`m5atom-matrix`](./m5atom-matrix)

A simple example to demonstrate built-in hardware features of the M5Atom Matrix development board.

***

### [`m5core_ink-clock`](./m5core_ink-clock)

Draws a basic digital clock on an ePaper display that updates once a minute. Only runs on M5 Paper and M5Core Ink because they are battery powered. Uses special power management techniques, including one to eliminate drawing flicker on the M5Core Ink.

> See the [blog post](ADDLINK) Getting the Most from ePaper Displays for more information about this example.

***

### [`m5stack-imu`](./m5stack-imu)

[Commodetto](../../documentation/commodetto/commodetto.md) app for the [M5Stack Fire](https://www.aliexpress.com/item/32847906756.html?spm=2114.12010615.8148356.14.11c127aeBNzJBb). Visualizes device movement based on data from the on-board InvenSense MPU-6050 accelerometer/gyro and Xtrinsic MAG3110 magnetometer.

***

### [`m5stickc-axp192`](./m5stickc-axp192)

[Commodetto](../../documentation/commodetto/commodetto.md) app for the [M5Stick-C](https://www.adafruit.com/product/4290). Demonstrates how to change the screen brightness to save power.

***

### [`m5stickc-imu`](./m5stickc-imu)

[Commodetto](../../documentation/commodetto/commodetto.md) app for the [M5Stick-C](https://www.adafruit.com/product/4290). Visualizes device movement based on data from the on-board SH200Q accelerometer/gyro.

***

### [`m5stickc-pedometer`](./m5stickc-pedometer)

[Commodetto](../../documentation/commodetto/commodetto.md) app for the [M5Stick-C](https://www.adafruit.com/product/4290). Combines the built-in screen and accelerometer on the [M5Stick-C](https://www.adafruit.com/product/4290) to create a simple pedometer.

***

### [`m5stickc-rtc`](./m5stickc-rtc)

[Commodetto](../../documentation/commodetto/commodetto.md) app for the [M5Stick-C](https://www.adafruit.com/product/4290). Demonstrates using the RTC BM8563 on the version 2 of the hardware.

***

### [`mcp23008`](./mcp23008)

Leverages the [MCP23008 GPIO expander](https://www.adafruit.com/product/593) to control multiple LEDs from a single I<sup>2</sup>C device.

***

### [`mcp23017`](./mcp23017)

Leverages the [MCP23017 GPIO expander](https://www.adafruit.com/product/732) to control multiple LEDs from a single I<sup>2</sup>C device.


***

### [`neopixel`](./neopixel)

Demonstrates use of the Moddable SDK NeoPixel API. This API works on ESP32 boards with integrated NeoPixels such as the M5Stack Fire or Lily Go TAudio, or you can combine an ESP32 board (like the NodeMCU ESP32) with a NeoPixel board from AdaFruit or Sparkfun. For more details, see [this blog post](https://blog.moddable.com/blog/neopixels/).

***

### [`neostrand`](./neostrand/docExample)

ESP32 example that animates WS2811 string lights. Making request to HTTP server switches between effects. For more information on this example and how to incorporate Neostrands into your projects, see [this blog post](https://blog.moddable.com/blog/a-very-neopixel-christmas/).

***

### [`onewire`](./onewire)

Reads the temperature from one or more DS18X20 temperature sensors connected to  the ESP8266 or ESP32. Tested with the [DS18B20](https://www.adafruit.com/product/374) and [DS18S20](https://www.mouser.com/ProductDetail/Maxim-Integrated/DS18S20%2BTR?qs=7H2Jq%252byxpJJMRp9%252bZx3PtA%3D%3D&gclid=EAIaIQobChMIwZzuu9OZ5AIViMBkCh0aoQ4tEAAYAiAAEgIZjfD_BwE) Digital temperature sensors. These sensors use the Dallas 1-Wire protocol, so they require just one digital pin for communication. Multiple sensors may be connected to the same pin.

***

### [`peripherals/rtc`](./peripherals/rtc)

A set of examples to demonstrate use of a real-time clock (RTC).

***

### [`qm1h0p0073`](./qm1h0p0073)
Reads the [Sharp Temperature and Humidity sensor](http://www.sharp-world.com/products/device/lineup/selection/rf/tem_hum/index.html) at 100ms intervals. The [driver](../../modules/drivers/qm1h0p0073) communicates with the sensor over I<sup>2</sup>C.

***

### [`qwiictwist`](./qwiictwist)

Demonstrates use of a [SparkFun Qwiic Twist](https://www.sparkfun.com/products/15083) (a digital RGB rotary encoder). Displays the numbers of twists and presses of the encoder and allows you to change its color using an onscreen color picker.

***

### [`radiotest`](./radio/radiotest)

Demonstrates how to use the [Mighty Gecko](https://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit) radio.

***

### [`redandblack`](./redandblack)
Renders bitmaps on ePaper displays compatible with the [DESTM32S](../../modules/drivers/destm32s) SPI ePaper display controller, e.g. the [Crystalfontz 128x296 3-Color ePaper Module](https://www.crystalfontz.com/product/cfap128296d00290-128x296-epaper-display).

***

### [`sakuraio`](./sakuraio)
Demonstrates how to interface with the [Sakura IO LTE module](http://python-sakuraio.readthedocs.io/en/latest/index.html) to send/receive packets and read module status. The [driver](../../modules/drivers/sakuraio) communicates with the sensor over I<sup>2</sup>C.

***

### [`sensors`](./sensors)

A set of examples for the following sensors, written in the TC53 [IO Class Pattern](../../documentation/io/io.md).

- `aht10`
- `am2320`
- `bmp180`
- `bmp280`
- `capacitiveMoisture`
- `ccs811`
- `hmc5883`
- `htu21d`
- `l3gd20`
- `lis3dh`
- `lm75`
- `lsm303dlhc`
- `mlx90614`
- `mpu6050`
- `mpu9250`
- `ntcThermistor`
- `sht3x`
- `shtc3`
- `si7020`
- `tmp102`
- `tmp117`
- `urm09`
- `zioQwiicSoilMoisture`

***

### [`si7021`](./si7021)

Reads the [SI7021 Temperature/Humidity sensor](https://www.adafruit.com/product/3251). This sensor is built into many of the Silicon Labs dev kits.

***

### [`sonoffb1`](./sonoffb1)

A collection of example apps for the [Sonoff B1 lightbulb](https://sonoff.ie/sonoff/54-sonoff-b1.html). For more information on installing Moddable SDK applications on the Sonoff B1, see [this blog post](https://blog.moddable.com/blog/hacking-sonoff-b1/).

***

### [`ssd1306`](./ssd1306)

[Commodetto](../../documentation/commodetto/commodetto.md) app that fills the screen with shades of gray. Works on SSD1306-compatible displays, such as the [SparkFun TeensyView](../../documentation/displays/wiring-guide-sparkFun-teensyview-spi.md).

***

### [`TMP102`](./TMP102)
Reads the [Texas Instruments TMP102 digital temperature sensor](https://www.sparkfun.com/products/13314) at 100ms intervals over I<sup>2</sup>C.

***

### [`xpt2046calibrate`](./xpt2046calibrate)
[Commodetto](../../documentation/commodetto/commodetto.md) app that calibrates the [XPT2046 touch screen controller](../../modules/drivers/xpt2046) used by the [Moddable Zero](../../documentation/etc/moddablezero.md) device. The app saves the calibration data in persistent storage.

***
