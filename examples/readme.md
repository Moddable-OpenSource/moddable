# Moddable SDK - Examples

Copyright 2018-2019 Moddable Tech, Inc.<BR>
Revised: May 17, 2019

The examples demonstrate how to use many of the capabilities of the Moddable SDK. Because each target platform is unique, not all examples run on every platform or device.

This document is a guide to building example applications in the Moddable SDK. It describes how to build apps using the `mcconfig` command line tool, configure builds for target platforms, and modify builds to support different screen/touch drivers and Wi-Fi configurations for microcontroller device targets.

> **Note**: This document assumes you have installed the Moddable SDK on your development machine. The [Getting Started](../documentation/Moddable%20SDK%20-%20Getting%20Started.md) document describes how to install the SDK.

## Table of Contents

* [Building apps](#building-apps)
* [Specifying target platforms](#target-platforms)
* [Screen pixel formats](#screen-pixel-formats)
* [Screen rotation](#screen-rotation)
* [Wi-Fi configuration](#wifi-configuration)
* [Screen driver configuration](#screen-driver-configuration)

<a id="building-apps"></a>
## Building apps

The `mcconfig` command line tool builds Moddable apps. To use it, open a terminal window, navigate into the app's directory, and build. For example, to run the `balls` app on macOS or Linux:

	cd $MODDABLE/examples/piu/balls
	mcconfig -m
	
On Windows, use the **Developer Command Prompt for VS 2017** to build apps:

	cd %MODDABLE%\examples\piu\balls
	mcconfig -m
	
By default, `mcconfig` generates a release build that targets the host platform. These are common command line options:

- `-d`: build a debug instrumented version
- `-i`: build a release instrumented version
- `-m`: run make automatically, otherwise `mcconfig` just generates the make file

For example, to build a debug version of an app that targets the host platform:

	mcconfig -d -m

Note that only a debug app build connects to `xsbug`, the XS JavaScript source level debugger.

Also note that `mcconfig` automatically deploys apps to target devices. For many platforms, the `xsbug` debugger is launched when deploying debug apps to devices.

> For additional details on `mcconfig` please refer to the [tools.md](../documentation/tools/tools.md) document.

<a id="target-platforms"></a>
### Specifying target platforms

The `-p` command line option specifies the target platform/subplatform you are building for. For example, to build a release app that targets Moddable One:

	mcconfig -m -p esp/moddable_one
	
To build a debug app that targets ESP32 devices:

	mcconfig -d -m -p esp32

For a full list of available platforms/subplatforms, see the **Platforms** section of the [Getting Started Guide](../documentation/Moddable%20SDK%20-%20Getting%20Started.md).

<a id="screen-pixel-formats"></a>
### Screen pixel formats

`mcconfig` defaults to building for 16-bit RGB565 little-endian pixels. Some example apps are designed to run on e-paper displays or LCD screens with different pixel formats. Use the `-f` command line option to set the destination screen pixel format:

- `-f rgb565le`: 16-bit RGB565 little-endian (default)
- `-f rgb332`: 8-bit RGB332
- `-f gray256`: 8-bit gray
- `-f clut16`: 4-bit indexed color
- `-f gray16`: 4-bit gray

For example, to build a debug version of the `balls` example app with the 8-bit gray pixel format for the host platform:

	cd $MODDABLE/examples/piu/balls
	mcconfig -d -m -f gray256

<a id="screen-rotation"></a>
### Screen rotation

Some example apps are designed to render on a rotated screen. Use the `-r` command line option to specify the target rotation angle: 

- `-r 0`: no rotation (default)
- `-r 90`: 90 degree rotation
- `-r 180`: 180 degree rotation
- `-r 270`: 270 degree rotation

For example, the `rotated` app is designed to run at 90 degree rotation. To build a debug version for [Moddable Two](../documentation/devices/moddable-two.md):

	cd $MODDABLE/examples/commodetto/rotated
	mcconfig -d -m -r 90 -p esp32/moddable_two


To build the `progress` app to run at 180 degrees rotation on the host platform:

	cd $MODDABLE/examples/commodetto/progress
	mcconfig -d -m -r 180
	
<a id="wifi-configuration"></a>
### Wi-Fi configuration

Example apps that require a network connection must configure the target device to connect to a Wi-Fi access point. Use the `ssid` and `password` configuration options to specify the access point credentials. The Moddable SDK runtime automatically tries to connect to the specified access point prior to launching the app.

> **Note**: Wi-Fi configuration is only supported for microcontroller targets.

For example, to build and run the `httpget` app on the network `friedkin` with a password `tacos`:

	cd $MODDABLE/examples/network/httpget
	mcconfig -d -m -p esp ssid=friedkin password=tacos

To build and run the `sntp` app for the open network `Free Wifi`:

	cd $MODDABLE/examples/network/sntp
	mcconfig -d -m -p esp ssid="Free WiFi"
	
<a id="screen-driver-configuration"></a>
### Screen driver configuration

Screen driver configuration is supported for microcontroller targets. Use the `screen` and `touch` configuration options to specify a screen or touch driver.

If you are building an application for any of the platforms listed in the **Target platforms** section above, you should not use the `screen` and `touch` configuration options; they will be configured automatically. But if you want to use a different display or touch driver, two steps are required:

1. Change the `screen` and `touch` configuration options to specify a screen or touch driver
2. Update the application's manifest to include the drivers

The remainder of this section explains how to do these steps.

#### Step 1: Using the `screen` and `touch` configuration options

To run the `text` app on an ESP8266 device with a [LPM013M126A](../modules/drivers/lpm013m126a) 8-color display:

	cd $MODDABLE/examples/commodetto/text
	mcconfig -d -m -p esp screen=lpm013m126a	
To run the `map-puzzle` app on an ESP8266 device with a [FT6206](../modules/drivers/ft6206) multi-touch controller:

	cd $MODDABLE/examples/piu/map-puzzle
	mcconfig -d -m -p esp touch=ft6206

#### Step 2: Including the drivers

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

To build an app for the `ft6206` touch controller driver, modify both the `config` and `include` sections in the `manifest.json` file:

	"config": {
		"screen": "ili9341",
		"touch": "ft6206"
	}
	
	"include": [
		"$(MODDABLE)/modules/drivers/ili9341/manifest.json",
		"$(MODDABLE)/modules/drivers/ft6206/manifest.json"
	]

> **Note**: For additional details on the `manifest.json` file please refer to the [manifest.md](../documentation/tools/manifest.md) document.
