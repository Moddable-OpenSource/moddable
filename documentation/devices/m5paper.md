# M5Paper Developer Guide 
Copyright 2021-2022 Moddable Tech, Inc.<BR>
Revised: March 22, 2022

This document provides information about using the M5Paper with the Moddable SDK, including how to build and deploy apps and links to other development resources.

<img src="./../assets/devices/m5paper.png" width=300>

## Table of Contents

- [SDK and Host Environment Setup](#setup)
- [Building and Deploying Apps](#building-and-deploying-apps)
- [Troubleshooting](#troubleshooting)
- [Development Resources](#development-resources)
	- [Port Status](#port-status)
	- [Display Driver](#display-driver) 
		- [Update Modes](#update-modes)
		- [Image Filters](#image-filters)
	- [Examples](#examples)
	- [Documentation](#documentation)
	- [Support](#support)
	- [Updates](#updates)

<a id="setup"></a>
## SDK and Host Environment Setup

To build and run apps on M5Paper, you'll need to:

1. Install the [Moddable SDK](./../Moddable%20SDK%20-%20Getting%20Started.md)
2. Install [ESP32 tools](./esp32.md)
3. (macOS users only) Follow the instructions in the **macOS** section below.
4. Follow the instructions in the **Building and Deploying Apps** section below.

<a id="macOS"></a>
### macOS

The USB driver situation for M5Paper on macOS is a little tricky. You need to:

- Run at least macOS Big Sur
- Install the driver referenced in this [issue](https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/issues/139#issuecomment-904390716)

<a id="building-and-deploying-apps"></a>
## Building and Deploying Apps

> There are several example applications in the Moddable SDK that show how to take make best use of the M5Paper. See the [ePaper blog](https://blog.moddable.com/blog/epaper#examples) post for details.

After you've set up your host environment and ESP32 tools, take the following steps to install an application on your M5Paper.

1. Attach the M5Paper to your computer with the USB cable that came with the device.

2. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Use the platform `-p esp32/m5paper`  with `mcconfig` to build for M5Paper. For example, to build the [`epaper-photos` example](../../examples/piu/epaper-photos):
	
	```text
	cd $MODDABLE/examples/piu/epaper-photos
	mcconfig -d -m -p esp32/m5paper
	```
	
	The [examples readme](../../examples) contains additional information about other commonly used `mcconfig` arguments for screen rotation, Wi-Fi configuration, and more.

	Use the platform `-p simulator/m5paper` with `mcconfig` to build for the M5Paper simulator.
	
<a id="troubleshooting"></a>
## Troubleshooting

See the Troubleshooting section of the [ESP32 documentation](./esp32.md) for a list of common issues and how to resolve them.

<a id="development-resources"></a>
## Development Resources

<a id="port-status"></a>
### Port Status

The following are implemented and working:

- EPD display driver
- GT911 touch driver
- SHT30 temperature/humidity sensor
- A / B / C buttons 
- RTC

> **Note**: The I2C address of the GT911 touch controller floats. The implementation tries both addresses 0x14 and 0x5D. This is handled in host provider's Touch constructor -- not in driver and not in user script. If 0x14 fails, an exception is thrown before it retries at 0x5D. If you encounter this, just hit Go in xsbug.

<a id="display-driver"></a>
### Display Driver

The display driver is a [Poco `PixelsOut`](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md#pixelsout-class) implementation. This allows it to use both the [Poco](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/poco.md) graphics APIs and[ Piu](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md) user interface framework from the Moddable SDK.

While many existing Poco and Piu examples run with the EPD, most are not practical. Because they were designed for a small color LCD with a high refresh rate, their appearance on a big gray display with a low refresh rate is often silly. We need some examples designed for this display.

The display driver is written entirely in JavaScript. It uses [Ecma-419 IO](https://419.ecma-international.org/#-9-io-class-pattern) APIs for all hardware access. Performance is excellent, often faster than the EPD class built into the native M5Paper library. One reason for this is that Poco can render directly to 4-bit gray pixels, eliminating the need for pixel format conversion. Another reason is that the SPI transfers to the display controller bulk transfer of thousands of pixels at a time, rather than four at a time. This reduces the number of bits transferred by over half.

Memory use is also quite low. There is no frame buffer in ESP32 memory: rendered pixels are sent directly to the display from a 16 line render buffer (about 8 KB).

Using the `continue` feature of Poco, it is possible to update several areas of the screen while only refreshing the EPD panel once. This allows for very efficient updates -- the least possible amount of memory is transferred and only one long panel flash occurs. The Piu `balls` example is a good way to see this in action -- only the ball images (not the empty space around them) are transferred to the display and only the rectangle that encloses the four balls flashes on the display panel.

The rotation feature of the display controller is supported, allowing no-overhead rotation at 0, 90, 180, and 270 degree rotations.

<a id="update-modes"></a>
#### Update Modes
The display controller supports several different [update modes](https://github.com/phoddie/m5paper/blob/f0b79e0a0579c0dbdb1bb4445dc6acf501403681/targets/m5paper/it8951.js#L82-L93). The optimal mode depends on the content being drawn. The mode may be changed on each frame. The default mode is `GLD16`. To change the mode, call the `config` method of the global `screen` object. For example:

```js
screen.config({updateMode: "A2"});
```

You may see artifacts that remain on the screen from previous apps when you install a new app on your device. To get rid of these, it is helpful to draw at least one complete frame in a high-quality mode (e.g. `GC16`) before switching to a faster update mode (e.g. `A2`). The [epaper-flashcards](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/piu/epaper-flashcards/main.js) example does this using a pattern that can be applied to most apps:

```js
onDisplaying(application) {
	screen.refresh?.();
	screen.configure?.({updateMode: config.firstDrawMode ?? config.updateMode});
	if (config.firstDrawMode)
		application.defer("onFinishedFirstDraw", config.updateMode);
	this.showNextCard(application, 1); // render the initial screen of the app
}
onFinishedFirstDraw(application, mode) {
	screen.configure({updateMode: mode});
}
```

Using this pattern, per-device `firstDrawMode` and `updateMode` settings can be applied in the project's `manifest.json`:

```json
"platforms": {
	"esp32/m5paper": {
		"config": {
			"firstDrawMode": "GC16",
			"updateMode": "A2"
		}
	}
}
```

<a id="image-filters"></a>
#### Image Filters
The display driver supports several different [pixel filters](https://github.com/phoddie/m5paper/blob/4110701c8084c07d7f777a44e17e970ffd18f729/targets/m5paper/it8951.js#L342-L349). These filter adjust the luminance of the pixels. The are useful for optimizing image and applying special effects. The default filter is "none". The filter may be changed on each frame. To change the filter, call the `config` method of the global `screen` object. For example:

```js
screen.config({filter: "negative"});
```

The filters are a `Uint8Array` of 16 values. To set your own filter, instead of using one of the built-in filters:

```js
let filter = new Uint8Array(16);
// code here to initialize filter
screen.config({filter});
```

<a id="examples"></a>
### Examples

The Moddable SDK has over 150 [example apps](../../examples) that demonstrate how to use its many features. Many of these examples run on M5Paper. 

That said, not every example is compatible with M5Paper hardware. For example, some examples are designed to test specific display and touch drivers that are not compatible with the M5Paper display and give a build error.

There are several example applications in the Moddable SDK that show how to take make best use of the M5Paper. See the [ePaper blog](https://blog.moddable.com/blog/epaper#examples) post for details.

<a id="documentation"></a>
### Documentation

All the documentation for the Moddable SDK is in the [documentation](../) directory. The **documentation**, **examples**, and **modules** directories share a common structure to make it straightforward to locate information. Some of the highlights include: 

- The `commodetto` subdirectory, which contains resources related to Commodetto--a bitmap graphics library that provides a 2D graphics API--and Poco, a lightweight rendering engine.
- The `piu` subdirectory, which contains resources related to Piu, a user interface framework that makes it easier to create complex, responsive layouts.
- The `networking` subdirectory, which contains networking resources related to BLE, network sockets, and a variety of standard, secure networking protocols built on sockets including HTTP/HTTPS, WebSockets, DNS, SNTP, and telnet.
- The `pins` subdirectory, which contains resources related to supported hardware protocols (digital, analog, PWM, I2C, etc.). A number of drivers for common off-the-shelf sensors and corresponding example apps are also available.

<a id="support"></a>
### Support

If you have questions, we recommend you [open an issue](https://github.com/Moddable-OpenSource/moddable/issues). We'll respond as quickly as practical, and other developers can offer help and benefit from the answers to your questions. Many questions have already been answered, so please try searching previous issues before opening a new issue.

<a id="updates"></a>
### Updates

The best way to keep up with what we're doing is to follow us on Twitter ([@moddabletech](https://twitter.com/moddabletech)). We post announcements about new posts on [our blog](http://blog.moddable.com/) there, along with other Moddable news.
