# Getting Started with Raspberry Pi Pico

Copyright 2021 Moddable Tech, Inc.<BR>
Revised: March 18, 2022

This document describes how to start building Moddable applications for the Raspberry Pi Pico. It provides information on how to configure host build environments, how to build and deploy apps, and includes links to external development resources.

> Note: The Pico port is solid and mostly complete. Future work to include:
> 
> - Mods
> - TC53 IO
> - PWM output
> - Audio
> - JavaScript Atomics
> - Web Workers making use of the second core
> - PIO integration
> - Building on Windows


## Table of Contents

- [About Raspberry Pi Pico](#about-pico)
- MacOS
 - [SDK and Host Environment Setup - macOS](#macos-setup)
 - [Building and Deploying apps - macOS](#macOS-building-and-deploying-apps)
- Windows
 - [SDK and Host Environment Setup - Windows](#windows-setup)
 - [Building and Deploying apps - Windows](#windows-building-and-deploying-apps)
- Linux
 - [SDK and Host Environment Setup - Linux](#linux-setup)
 - [Building and Deploying apps - Linux](#linux-building-and-deploying-apps)

- [Debugging Native Code](#debugging-native-code)
- [Reference Documents](#reference)

<a id="about-pico"></a>
## About Raspberry Pi Pico

<img src="../assets/devices/pi-pico.png" width="300">

Please see the [Raspberry Pi Pico documentation](https://www.raspberrypi.org/documentation/pico/getting-started/) for details.

<a id="setup"></a>
## SDK and Host Environment Setup

<a id="macos-setup"></a>

### macOS Setup

1. The [Moddable SDK Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md) describes how to configure the host build environment and install the required SDKs, drivers, and development tools. Follow the instructions in the [Host environment setup](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md#host-mac) section for macOS.


2. Set up the Pico SDK
> The next steps are condensed from the [Raspberry Pi Pico C SDK][picosdkdoc] document. Refer to the document for complete instructions.

	Create a `pico` directory in your home directory at `~/pico` for required third party SDKs and tools.

	```text
	cd $HOME
	mkdir pico
	```

3. Install required components using `brew`.

	```text
	brew install cmake
	brew tap ArmMbed/homebrew-formulae
	brew install arm-none-eabi-gcc
	```

4. Install the __pico__ sdk and examples:

	```text
	cd $HOME/pico
	git clone -b master https://github.com/raspberrypi/pico-sdk
	cd pico-sdk
	git submodule update --init
	```

	```text
	cd $HOME/pico
	git clone -b master https://github.com/raspberrypi/pico-examples
	```
	
6. Set the `PICO_SDK_PATH` environment variable to point to the Pico SDK directory:

	```text
	export PICO_SDK_PATH=$HOME/pico/pico-sdk
	```

7. Build some `pico` tools:

	```text
	cd $PICO_SDK_PATH
	mkdir build
	cd build
	cmake ..
	make
	```


<a id="macOS-building-and-deploying-apps"></a>
#### Building and Deploying Apps on macOS

After you've setup your macOS host environment, take the following steps to install an application on your Pico.

1. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Specify the platform `-p pico` with `mcconfig` to build for the Pico. Build the [`helloworld`](../../examples/helloworld) example:
	
	```text
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p pico
	```

The app will be built and installed. `xsbug` will be launched and connected to the Pico after a few seconds.

> Note: If the device is unresponsive, you may see this message:
>	
>	```text
>	Hold the BOOTSEL button and power-cycle the device.
>	Waiting for /Volumes/RPI-RPI2.....
>	```
>
>  Put the device into programming mode by holding the __BOOTSEL__ button when powering on the Pico.
>
>    You will know that programming mode is active when a disk named `RPI-RP2` appears on your desktop.


<a id="windows-setup"></a>

### Windows Setup

Not yet available.

<a id="win-building-and-deploying-apps"></a>
#### Building and Deploying Apps on Windows

Not yet available.


<a id="linux-setup"></a>

### Linux setup

> Note: This setup was performed on a Ubuntu 20 VM using VirtualBox.

1. The [Moddable SDK Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md) describes how to configure the host build environment and install the required SDKs, drivers, and development tools. Follow the instructions in the [Host environment setup](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md#lin-instructions) section for Linux.


2. Set up the Pico SDK
> The next steps are condensed from the [Raspberry Pi Pico C SDK][picosdkdoc] document. Refer to the document for complete instructions.

	Create a `pico` directory in your home directory at `~/pico` for required third party SDKs and tools.

	```text
	cd $HOME
	mkdir pico
	```

3. Install required components using `apt`.

	```text
	sudo apt update
	sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
	```

4. Set `PICO_GCC_ROOT` environment variable to point to the `bin` directory of your `arm-none-eabi` toolchain. For Ubuntu 20, it is set to `/usr`.

	```text
	export PICO_GCC_ROOT=/usr
	```

5. Install the __pico__ sdk and examples:

	```text
	cd $HOME/pico
	git clone -b master https://github.com/raspberrypi/pico-sdk
	cd pico-sdk
	git submodule update --init
	```

	```text
	cd $HOME/pico
	git clone -b master https://github.com/raspberrypi/pico-examples
	```
	
6. Set the `PICO_SDK_PATH` environment variable to point to the Pico SDK directory:

	```text
	export PICO_SDK_PATH=$HOME/pico/pico-sdk
	```

7. Build some `pico` tools:

	```text
	cd $PICO_SDK_PATH
	mkdir build
	cd build
	cmake ..
	make
	```


<a id="linux-building-and-deploying-apps"></a>
### Building and Deploying Apps on Linux

After you've setup your Linux host environment, take the following steps to install an application on your Pico.

1. Put the device into programming mode by holding the __BOOTSEL__ button when powering on the Pico.

	Make sure you're using a data-sync capable cable, not one that is power-only.

	> Note: a USB hub with power switch is very helpful here.

	You will know that programming mode is active when a disk named `RPI-RP2` appears on your desktop.
	
	<img src="../assets/devices/pico-on-linux.png" width="175">

	> Note: For best results with a virtual machine, capture the Pico device in both the Boot mode state and running state. The image below shows the configuration in VirtualBox:
	
	<img src="../assets/devices/pico-vbox-usb.png" width="400">

3. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Specify the platform `-p pico` with `mcconfig` to build for the Pico. Build the [`helloworld`](../../examples/helloworld) example:
	
	```text
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p pico
	```

The app will be built and installed. `xsbug` will be launched and connected to the Pico after a few seconds.



<a id="debugging-native-code"></a>
## Debugging Native Code

Refer to the [Getting Started With Pico][picogettingstarteddoc] for 
instructions on setting up your hardware.

These instructions have been tested on a macOS host using the two Pico SWD setup described in Appendix A: Using Picoprobe.

1. Build pico-openocd as described in the document.

2. In a console, start `openocd` and set the console aside.

	```text
	cd ~/pico/openocd
	~/pico/openocd/src/openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -s tcl
	```

3. In another console, change the directory to the build results directory and start `gdb`:

	```text
	cd $MODDABLE/build/bin/pico/debug/<app>
	arm-none-eabi-gdb xs_pico.elf
	```

4. Connect to `openocd`. `load` the app. `reset` the device.

	```text
	(gdb) target remote localhost:3333
	(gdb) load
	(gdb) monitor reset init
	(gdb) continue
	```
	
<a id="reference"></a>
## Reference Documents

[Getting started with Raspberry Pi Pico][picogettingstarteddoc]

[Hardware Design with RP2040][picohwdoc]

[Raspberry Pi Pico C SDK][picosdkdoc]
	

[picogettingstarteddoc]:https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf
[picohwdoc]:https://datasheets.raspberrypi.org/rp2040/hardware-design-with-rp2040.pdf
[picosdkdoc]:https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf
