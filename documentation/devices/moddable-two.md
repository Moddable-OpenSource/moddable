# Getting Started with Moddable Two

Copyright 2019 Moddable Tech, Inc.<BR>
Revised: December 10, 2019

This document describes how to start building Moddable applications for Moddable Two. It provides information on how to configure the host build environment and how to build and deploy apps. It also provides information about development resources, including a summary of the examples available in this repository that run on Moddable Two.

## Table of Contents

- [About Moddable Two](#about-moddable-two)
	- [Components](#components)
	- [Pinout](#pinout)
- [SDK and Host Environment Setup](#setup)

- [Building and Deploying Apps](#building-and-deploying-apps)

- [Development Resources](#development-resources)
	- [Examples](#examples)
	- [Documentation](#documentation)
	- [Backlight](#backlight)
	- [Support](#support)
	- [Updates](#updates)

<a id="about-moddable-two"></a>
## About Moddable Two

<img src="../assets/devices/moddable-two.png">

Moddable Two is a hardware module that makes it easy for developers to experiment with the Moddable SDK on inexpensive hardware. It is available to purchase on the [Moddable website](http://www.moddable.com/moddable-two).

<a id="components"></a>
### Components

The two main components of Moddable Two are the ESP32 module and capacitive touch screen. The ESP32 module includes a Wi-Fi/BLE antenna and 4 MB of flash storage memory. The touch screen is a 240 x 320 QVGA IPS display driven by a MIPI Display Serial Interface compatible display controller with an FT6206 capacitive touch controller.

<a id="pinout"></a>
### Pinout

<img src="../assets/devices/moddable-two-pinout.png">

<a id="setup"></a>
## SDK and Host Environment Setup

The [Moddable SDK Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md) describes how to configure the host build environment and install the required SDKs, drivers, and development tools. Follow the instructions in the **Host environment setup** and **ESP32 setup** sections for your computer's operating system.

<a id="building-and-deploying-apps"></a>
## Building and Deploying Apps

After you've set up your host environment, take the following steps to install an application on your Moddable Two.

1. Attach the programmer to your Moddable Two.

	Make sure you have the programmer oriented correctly. The orientation should match the image below.
	
	<img src="../assets/devices/moddable-two-programmer.jpg">

	**Note**: The USB port on Moddable Two may be used to provide power when operating without the programmer. The USB port is only for powering Moddable Two. It cannot be used to program Moddable Two.

2. Attach the programmer to your computer with a micro USB cable.

	The USB cable must be attached to the programmer, not the power-only USB port on the board. Make sure you're using a data sync&#8211;capable cable, not one that is power-only.

3. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Use the platform `-p esp32/moddable_two`  with `mcconfig` to build for Moddable Two. For example, to build the [`piu/balls` example](../../examples/piu/balls):
	
	```
	cd $MODDABLE/examples/piu/balls
	mcconfig -d -m -p esp32/moddable_two
	```
	
	The [examples readme](../../examples) contains additional information about other commonly used `mcconfig` arguments for screen rotation, Wi-Fi configuration, and more.

<a id="development-resources"></a>
## Development Resources

<a id="examples"></a>
### Examples

The Moddable SDK has over 150 [example apps](../../examples) that demonstrate how to use its many features. The vast majority of these examples run on Moddable Two. 

That said, not every example is compatible with Moddable Two hardware. For example, some examples are designed to test specific display and touch drivers that are not compatible with the Moddable Two display and give a build error.

<a id="documentation"></a>
### Documentation

All the documentation for the Moddable SDK is in the [documentation](../) directory. The **documentation**, **examples**, and **modules** directories share a common structure to make it straightforward to locate information. Some of the highlights include: 

- The `commodetto` subdirectory, which contains resources related to Commodetto--a bitmap graphics library that provides a 2D graphics API--and Poco, a lightweight rendering engine.
- The `piu` subdirectory, which contains resources related to Piu, a user interface framework that makes it easier to create complex, responsive layouts.
- The `networking` subdirectory, which contains networking resources related to BLE, network sockets, and a variety of standard, secure networking protocols built on sockets including HTTP/HTTPS, WebSockets, DNS, SNTP, and telnet.
- The `pins` subdirectory, which contains resources related to supported hardware protocols (digital, analog, PWM, I2C, etc.). A number of drivers for common off-the-shelf sensors and corresponding example apps are also available.

<a id="backlight"></a>
### Backlight
The original Moddable Two has an always-on backlight. The section revision has the ability to adjust the backlight brightness in software.  Moddable Two units with backlight brightness control are identified by the a small "`ESP32 r9`" printed on the board to the right of the Moddable logo. 

The backlight control is connected to GPIO 18. There is a constant defined for the backlight GPIO in the host config.

```javascript
import "config" from "mc/config";

Digital.write(config.backlight, 1);	// backlight ON
Digital.write(config.backlight, 0);	// backlight OFF
```

The brightness of the backlight may be set at build time in the `config` section of your project manifest. It defaults to 100%.

```
"config": {
	"brightness": 75,
},
```

You can also set the brightness on the command line when building with `mcconfig`. Here it is set to 50%.

```
mcconfig -d -m -p esp32/moddable_two backlight=50
```

The `setup/target` module for Moddable Two installs a global variable named `backlight` that you can use to adjust the backlight in your code. Here it is set to 80%.

```javascript
backlight.write(80);
```

The `backlight` global contains an instance of a subclass of `PWM`. If you do not want the `setup/target` to create this `PWM` instance, set the backlight to `"none"` in the `config` section of your project's manifest.

```
"config": {
	"brightness": "none",
},
```

**Note**: Backlight support is present in all builds using the `esp32/moddable_two` build target, however it only works for revision two. The original Moddable Two has an always on backlight.

<a id="support"></a>
### Support

If you have questions, we recommend you [open an issue](https://github.com/Moddable-OpenSource/moddable/issues). We'll respond as quickly as practical, and other developers can offer help and benefit from the answers to your questions. Many questions have already been answered, so please try searching previous issues before opening a new issue.

<a id="updates"></a>
### Updates

The best way to keep up with what we're doing is to follow us on Twitter ([@moddabletech](https://twitter.com/moddabletech)). We post announcements about new posts on [our blog](http://blog.moddable.com/) there, along with other Moddable news.