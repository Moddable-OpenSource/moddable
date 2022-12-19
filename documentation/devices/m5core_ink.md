# M5Core Ink Developer Guide
Copyright 2021 Moddable Tech, Inc.<BR>
Revised: November 2, 2021

This document provides information about using the M5Core Ink with the Moddable SDK, including how to build and deploy apps and links to other development resources.

<img src="./../assets/devices/m5coreink.png" width=250>

## Table of Contents

- [SDK and Host Environment Setup](#setup)
- [Building and Deploying Apps](#building-and-deploying-apps)
- [Troubleshooting](#troubleshooting)
- [Development Resources](#development-resources)
	- [Port Status](#port-status)
	- [Display Driver](#display-driver) 
	- [Buttons](#buttons)
	- [LED](#led)
	- [Buzzer](#buzzer)
	- [Battery Voltage](#battery-voltage)
	- [Battery Powered Operation](#battery-powered-operation)
	- [Examples](#examples)
	- [Documentation](#documentation)
	- [Support](#support)
	- [Updates](#updates)

<a id="setup"></a>
## SDK and Host Environment Setup

To build and run apps on M5Core Ink, you'll need to:

1. Install the [Moddable SDK](./../Moddable%20SDK%20-%20Getting%20Started.md)
2. Install [ESP32 tools](./esp32.md)
3. Follow the instructions in the **Building and Deploying Apps** section below.

<a id="building-and-deploying-apps"></a>
## Building and Deploying Apps

> There are several example applications in the Moddable SDK that show how to take make best use of the M5Core Ink. See the [ePaper blog](https://blog.moddable.com/blog/epaper#examples) post for details.

After you've set up your host environment and ESP32 tools, take the following steps to install an application on your M5Core Ink.

1. Attach the M5Core Ink to your computer with the USB cable that came with the device.

2. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Use the platform `-p esp32/m5core_ink`  with `mcconfig` to build for M5Core Ink. For example, to build the [`epaper-photos` example](../../examples/piu/epaper-photos):
	
	```text
	cd $MODDABLE/examples/piu/epaper-photos
	mcconfig -d -m -p esp32/m5core_ink
	```
	
	The [examples readme](../../examples) contains additional information about other commonly used `mcconfig` arguments for screen rotation, Wi-Fi configuration, and more.

<a id="troubleshooting"></a>
## Troubleshooting

See the Troubleshooting section of the [ESP32 documentation](./esp32.md) for a list of common issues and how to resolve them.

<a id="development-resources"></a>
## Development Resources

<a id="port-status"></a>
### Port Status

The following are implemented and working:

- EPD display driver (GDEW0154M09)
- RTC (PCF8563 / BM8563)
- Up / Down / Middle / Power / External buttons 
- LED
- Buzzer
- Battery voltage

<a id="display-driver"></a>
### Display Driver

The display driver is a [Poco `PixelsOut`](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md#pixelsout-class) implementation. This allows it to use both the [Poco](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/poco.md) graphics APIs and[ Piu](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md) user interface framework from the Moddable SDK.

The display driver is written entirely in JavaScript. It uses [Ecma-419 IO](https://419.ecma-international.org/#-9-io-class-pattern) APIs for all hardware access. Performance is excellent, often faster than the EPD class built into the native M5Core Ink library. 

The display driver implements dithering, which allows many levels of gray to be displayed using only black and white pixels. The default dithering algorithm is the venerable [Atkinson dither](https://twitter.com/phoddie/status/1274054345969950720). To change to Burkes or to disable dithering:

```js
screen.dither = "burkes";
screen.dither = "none"
```

Dithering is performed using the new `commodetto/Dither` module.

To support dithering the driver requires that Poco render at 8-bit gray (256 Gray). The driver also only supports full screen updates as partial updates with dithering show seams. Partial updates could, and probably should, be supported as they could be useful when not using dithering.

The driver maintains a back buffer, which is more-or-less necessary because of the way the display controller works. Fortunately the buffer is just 5000 bytes, since it can use 1-bit pixels.

The display driver does a full screen refresh on the first draw after instantiation. To disable this, set `refresh` to false.

```js
screen.configure({refresh: false});
```

The display driver uses partial updates after the first frame. To force a full screen update: 

```js
screen.configure({refresh: true});
```

A partial update may be performed on power-up to avoid an initial full screen flash. To do this, the previous frame must first be redrawn to put it back into the controller's memory. To do that, first draw the previous frame with `previous` set to true, then draw the next frame as usual. The driver resets the value of `previous` to `false` after one frame is drawn.

```js
screen.configure({previous: true, refresh: false});
// draw previous
// draw next

```

Hardware rotation is not supported. It could be, but given that the display is square it isn't obviously useful. Both Poco and Piu support software rotation at build time, so rotation is available if needed, just not at runtime.

<a id="buttons"></a>
### Buttons

All of the buttons are available with callbacks when changed. Here's a basic test that installs callbacks on each.

```js
for (let name in device.peripheral.button) {
	new device.peripheral.button[name]({
		onPush() {
			trace(`Button ${name}: ${this.pressed}\n`);
		}
	})
}
```

<a id="led"></a>
### LED

The green LED at the top of the unit is available:

```js
const led = new device.peripheral.led.Default;
led.on = 1;		// full strength
led.on = 0;		// off
led.on = 0.5;		// half strength
```

<a id="buzzer"></a>
## Buzzer

The buzzer is implemented to play tones. As with the M5 Speaker API, sounds are played immediately.

Instantiate the buzzer:

```js
const buzzer = new device.peripheral.tone.Default;
```

Play a note by name and octave:

```js
buzzer.note("C", 4);
```

Play a note by name and octave for a fixed duration in milliseconds:

```js
buzzer.note("Bb", 4, 500);
```

Play a tone by frequency:

```js
buzzer.tone(262);
```

Play a tone by frequency for a fixed duration in milliseconds:

```js
buzzer.tone(262, 500);
```

Mute the buzzer (it automatically unmutes on the next note or tone played):

```js
buzzer.mute();
```

Close the buzzer:

```js
buzzer.close();
```

<a id="battery-voltage"></a>
### Battery Voltage

To get the battery voltage:

```js
const battery = new device.peripheral.battery.Default;
const voltage = battery.read();
```

<a id="battery-powered-operation"></a>
### Battery Powered Operation

To operate M5Core Ink on battery power, the power hold line must be set to 1. This is done by default in the `setup/target` module.

```js
power.main.write(1);
```

To turn the device off when running on battery power, set the power hold line to 0.

```js
power.main.write(0);
```

<a id="examples"></a>
### Examples

The Moddable SDK has over 150 [example apps](../../examples) that demonstrate how to use its many features. Many of these examples run on M5Core Ink. 

That said, not every example is compatible with M5Core Ink hardware. For example, some examples are designed to test specific display and touch drivers that are not compatible with the M5Core Ink display and give a build error.

The Moddable SDK Piu examples that do not depend on touch generally seem to work as-is, though some don't look their best on an ePaper display. The display refresh rate on the M5Core Ink is about 3 FPS, so examples like `balls` will not look good.

There are several example applications in the Moddable SDK that show how to take make best use of the M5Core Ink. See the [ePaper blog](https://blog.moddable.com/blog/epaper#examples) post for details.

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
