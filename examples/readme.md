# Moddable SDK - Examples

Copyright 2017 Moddable Tech, Inc.

<!-- Last edit: 11/29/2017 BSF -->

Revised: November 29, 2017

The examples demonstrate how to use many of the capabilities of the Moddable SDK. Because each target platform is unique, not all examples run on every platform or device.

This document is a guide to building example applications in the Moddable SDK. It describes how to build apps using the `mcconfig` command line tool, configure builds for target platforms, and modify builds to support different screen/touch drivers and Wi-Fi configurations for microcontroller device targets.

> **Note**: This document assumes you have installed the Moddable SDK on your development machine. The [Getting Started](documentation/Moddable%20SDK%20–%20Getting%20Started.md) document describes how to install the SDK.

## Building apps

The `mcconfig` command line tool builds Moddable apps. Open a terminal window, navigate into the app's directory and build. For example, the following shows how to build and run the `balls` app on macOS or Linux:

	cd $MODDABLE/examples/piu/balls
	mcconfig -m
	
On Windows, use the **Developer Command Prompt for VS 2017** to build apps:

	cd %MODDABLE%\examples\piu\balls
	mcconfig -m
	
By default, `mcconfig` generates a release build that targets the host platform. These are common command line options:

- `-d`: build a debug instrumented version
- `-i`: build a release instrumented version
- `-m`: run make automatically, otherwise `mcconfig` just generates the make file

To build a debug version of an app that targets the host platform:

	mcconfig -d -m

>**Note**: Only a debug app build connects to the `xsbug`, the XS JavaScript source level debugger.

For additional details on `mcconfig` please refer to the [tools.md](https://github.com/Moddable-OpenSource/moddable/blob/master/documentation/tools/tools.md) document.


### Target platforms

The `-p` command line option specifies the target platform you are building for:

- `-p mac`: macOS target
- `-p win`: Windows target
- `-p lin`: Linux target
- `-p esp`: ESP8266 device target
- `-p esp32`: ESP32 device target 

To build a release app that targets ESP8266 devices:

	mcconfig -m -p esp
	
To build a debug app that targets ESP32 devices:

	mcconfig -d -m -p esp32
	
> **Note**: `mcconfig` automatically deploys apps to target devices. The `xsbug` debugger is launched when deploying debug apps to devices.

### Screen pixel formats

`mcconfig` defaults to building for 16-bit RGB565 little-endian pixels. Some example apps are designed to run on e-Ink displays or LCD screens with different pixel formats. Use the `-f` command line option to set the destination screen pixel format:

- `-f rgb565le`: 16-bit RGB565 little-endian (default)
- `-f rgb332`: 8-bit RGB332
- `-f gray256`: 8-bit gray
- `-f clut16`: 4-bit indexed color
- `-f gray16`: 4-bit gray

To build a release version of the `redandblack` e-Ink example app for the RGB332 [Crystalfontz 3-Color display](https://www.crystalfontz.com/product/cfap128296d00290-128x296-epaper-display) for an ESP8266 device:

	cd $MODDABLE/examples/drivers/redandblack
	mcconfig -m -f rgb332 -p esp
	
### Screen rotation

Some example apps are designed to render on a rotated screen. Use the `-r` command line option to specify the target rotation angle: 

- `-r 0`: no rotation (default)
- `-r 90`: 90 degree rotation
- `-r 180`: 180 degree rotation
- `-r 270`: 270 degree rotation

To build a debug version of the `rotated` app, designed to run at 90 degree rotation for the ESP32 device:

	cd $MODDABLE/examples/commodetto/rotated
	mcconfig -d -m -r 90 -p esp32
	
To build the `love-e-ink` app, designed to run rotated 90 degress on a 1-bit e-Ink display:

	cd $MODDABLE/examples/piu/love-e-ink
	mcconfig -d -m -f gray256 -r 90 -p esp

To build the `progress` app to run at 180 degrees rotation on the host platform:

	cd $MODDABLE/examples/commodetto/progress
	mcconfig -d -m -r 180
	
### Wi-Fi configuration

Example apps that require a network connection must configure the target device to connect to a Wi-Fi access point. Use the `ssid` and `password` configuration options to specify the access point credentials. The Moddable SDK runtime automatically tries to connect to the specified access point prior to launching the app.

> **Note**: Wi-Fi configuration is only supported for microcontroller targets.

To build and run the `httpget` app on the network `friedkin` with a password `tacos`:

	cd $MODDABLE/examples/network/httpget
	mcconfig -d -m -p esp ssid=friedkin password=tacos

To build and run the `sntp` app for the open network `Free Wifi`:

	cd $MODDABLE/examples/network/sntp
	mcconfig -d -m -p esp ssid="Free WiFi"
	
### Screen driver configuration

Most of the graphics example apps are pre-configured to run on ESP8266 devices with a LCD screen compatible with the [ILI9341](../modules/drivers/ili9341) driver and [XPT2046](../modules/drivers/xpt2046) touch controller. To use a different display or touch driver, two steps are required. First, change `screen` and `touch` configuration options to specify a screen or touch driver. Second, update the application's manifest to include the drivers. The remainder of this section explains how to do these steps.

> **Note**: Screen driver configuration is only supported for microcontroller targets.

To run the `text` app on an ESP8266 device with a [LPM013M126A](../modules/drivers/lpm013m126a) 8-color display:

	cd $MODDABLE/examples/commodetto/text
	mcconfig -d -m -p esp screen=lpm013m126a	
To run the `map-puzzle` app on an ESP8266 device with a [FT6206](../modules/drivers/ft6206) multi-touch controller:

	cd $MODDABLE/examples/piu/map-puzzle
	mcconfig -d -m -p esp touch=ft6206
	
Each application includes a `manifest.json` file that describes the modules and resources required to build that app. The `config` section specifies the screen and touch drivers used by the application. Note the `screen` and `touch` configuration options specified on the command line override those found in the manifest:

	"config": {
		"screen": "ili9341",
		"touch": "xpt2046"
	}

The `include` section specifies which driver modules are built for the application:

	"include": [
		"$(MODDABLE)/modules/drivers/ili9341/manifest.json",
		"$(MODDABLE)/modules/drivers/xpt2046/manifest.json"
	]

To build an app that for the `ft6206` touch controller driver, modify both the `config` and `include` sections in the `manifest.json` file:

	"config": {
		"screen": "ili9341",
		"touch": "ft6206"
	}
	
	"include": [
		"$(MODDABLE)/modules/drivers/ili9341/manifest.json",
		"$(MODDABLE)/modules/drivers/ft6206/manifest.json"
	]

> **Note**: For additional details on the `manifest.json` file please refer to the [manifest.md](../documentation/tools/manifest.md) document.
