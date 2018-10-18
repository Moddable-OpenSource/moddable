# Moddable SDK - Driver Examples

Copyright 2017 Moddable Tech, Inc.

<!-- Last edit: 12/6/2017 BSF -->

Revised: December 6, 2017

These examples demonstrate how to use the Moddable SDK to write drivers and applications that interface with a variety of components. This document provides a brief description of each example and components supported.

> **Note**: This document assumes you have installed the Moddable SDK on your development machine. The [Getting Started](../../documentation/Moddable%20SDK%20-%20Getting%20Started.md) document describes how to install the SDK.

### [arducamframeserver](./arducamframeserver)

HTTP server that returns a JPEG image captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/).

### [arducamhttpput](./arducamhttpput)

HTTP client that posts successive JPEG images captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/) to a HTTP server.

### [arducampreview](./arducampreview)

Displays a raw 16-bit RGB image captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/) on a LCD screen compatible with the [ILI9341](../../modules/drivers/ili9341) driver.

### [arducamstreamserver](./arducamstreamserver)

HTTP server that returns a JPEG image captured from the [ArduCAM Mini 2MP SPI camera](http://www.arducam.com/arducam-mini-released/) as a multipart HTTP response. The HTTP client can optionally specify the image width and height in the request.

### [BMP180](./BMP180)

HTTP server that returns a text reading from an I<sup>2</sup>C [Bosch BMP180 Barometric Pressure/Temperature sensor](https://www.adafruit.com/product/1603). The application additionally configures the host as a Wi-Fi base station.

### [dotstar](./dotstar)

Progressively renders each row of a color image to an [Adafruit DotStar 144 LED strip](https://www.adafruit.com/product/2242). The [driver](../../modules/drivers/dotstar) communicates with the strip over SPI.

### [ga1auv100wp](./ga1auv100wp)
Reads ambient and UV light measurements from the dual-mode [Sharp UV Light Sensor](http://www.sharp-world.com/products/device/lineup/selection/opto/receiving_light/opic/ultraviolet/index.html). The application configures the sensor operating mode. The [driver](../../modules/drivers/ga1auv100wp) communicates with the sensor over I<sup>2</sup>C using the SMBus protocol.

### [gp2ap01vt00f](./gp2ap01vt00f)
Reads distance measurements at 100ms intervals from the [Sharp Time-of-Flight Laser Ranging Sensor](http://www.sunnic.com/upfiles/tw_/sharp/GP2AP01VT00F.pdf). The [driver](../../modules/drivers/gp2ap01vt00f) communicates with the sensor over I<sup>2</sup>C.

### [HMC5883L](./HMC5883L)

Reads digital compass measurements at 250ms intervals from the I<sup>2</sup>C [SparkFun Triple Axis Magnetometer Breakout - HMC5883L](https://www.sparkfun.com/products/retired/10530).	
### [lis3dh](./lis3dh)

Reads the [Adafruit LIS3DH Triple-Axis Accelerometer](https://www.adafruit.com/product/2809) at 100ms intervals. The [driver](../../modules/drivers/lis3dh) communicates with the accelerometer over I<sup>2</sup>C.

### [lis3dhball](./lis3dhball)

[Commodetto](../../documentation/commodetto/commodetto.md) app that displays [Adafruit LIS3DH Triple-Axis Accelerometer](https://www.adafruit.com/product/2809) sensor values in real time and animates a bouncing ball based on the X and Y readings.

### [mcp23008](./mcp23008)

Leverages the [MCP23008 GPIO expander](https://www.adafruit.com/product/593) to control up to eight LEDs from a single I<sup>2</sup>C [device](../../modules/drivers/mcp23008).

### [qm1h0p0073](./qm1h0p0073)
Reads the [Sharp Temperature and Humidity sensor](http://www.sharp-world.com/products/device/lineup/selection/rf/tem_hum/index.html) at 100ms intervals. The [driver](../../modules/drivers/qm1h0p0073) communicates with the sensor over I<sup>2</sup>C.

### [redandblack](./redandblack)
Renders bitmaps on ePaper displays compatible with the [DESTM32S](../../modules/drivers/destm32s) SPI ePaper display controller, e.g. the [Crystalfontz 128x296 3-Color ePaper Module](https://www.crystalfontz.com/product/cfap128296d00290-128x296-epaper-display).

### [sakuraio](./sakuraio)
Demonstrates how to interface with the [Sakura IO LTE module](http://python-sakuraio.readthedocs.io/en/latest/index.html) to send/receive packets and read module status. The [driver](../../modules/drivers/sakuraio) communicates with the sensor over I<sup>2</sup>C.

### [TMP102](./TMP102)
Reads the [Texas Instruments TMP102 digital temperature sensor](https://www.sparkfun.com/products/13314) at 333ms intervals over I<sup>2</sup>C.

### [xpt2046calibrate](./xpt2046calibrate)
[Commodetto](../../documentation/commodetto/commodetto.md) app that calibrates the [XPT2046 touch screen controller](../../modules/drivers/xpt2046) used by the [Moddable Zero](../../documentation/etc/moddablezero.md) device. The app saves the calibration data in persistent storage.
