# Moddable Four Getting Started

Copyright 2020 Moddable Tech, Inc.

Revised: February 5, 2020

Warning: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## About This Document

This document describes how to set up and build Moddable applications for the Nordic nRF52 series processors using the **gcc** tools. It is specifically written for the **Moddable Four**, and can be adapted for other nRF52840 devices.

## Table of Contents

* [About Moddable Four](#about-moddable-four)
* [Development Workflow](#development-workflow)
* [Getting Started](#getting-started)
  * [MacOS host environment setup](#macos-host-environment-setup)
    * [Install a gcc toolchain](#install-a-gcc-toolchain)
    * [Install the nRF Command Line Tools](#install-the-nrf-command-line-tools)
    * [Get uf2conv.py](#get-uf2conf.py)
* [nRF5 SDK Setup](#nrf5-sdk-setup)
  * [Install the nRF5 SDK v15.3.0](#install-the-nrf5-sdk-v15.3.0)
  * [Add a board definition](#add-a-board-definition)
* [MacOS Build and Deploy](#macos-build-and-deploy)
  * [Build Moddable tools for MacOS](#build-moddable-tools-for-macos)
  * [Moddable Four Programming mode](#moddable-four-programming-mode)
  * [Build and deploy a Moddable app](#build-and-deploy-a-moddable-app)
* [Debugging](#debugging)
  * [Wiring the serial interface](#wiring-the-serial-interface)
  * [Start xsbug](#start-xsbug)
  * [Native debugging on Moddable Four](#native-debugging-on-moddable-four)
* [Bootloader](#bootloader)
* [Notes and troubleshooting](#notes-and-troubleshooting)
* [Example apps with nrf52 support](#example-apps-with-qca4020-support)
* [Reference Documentation](#reference-documentation)
* [Reference Sites](#reference-sites)


## About Moddable Four

Moddable's Moddable Four device is a low-power, bluetooth development board that utilizes Nordic Semiconductor's nRF52840 hardware and Moddable's ECMAScript software development tools.

The Nordic nRF52840 is an low-powered ARM-based SoC with Bluetooth. The Moddable SDK can also be used with the [nRF52840 DK](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK) as well as other nRF52840 based boards.

Moddable supports FreeRTOS on the nRF52 using a MacOS based host build platform.

The build platform `-p nrf52/moddable_four` is used to target the Moddable Four. An alternate build arrangement can be set up for development with Nordic's **nRF52840-DK** development board and Segger Embedded Systems IDE. See the Using Moddable with Segger Embedded Systems document for details.

## Development workflow

After the initial build host setup, building and deploying to the Moddable Four can be performed with the command:

```
mcconfig -d -m -p nrf52/moddable_four -t copyToM4
```

This will perform the following steps:

1. The application, assets, and modules are built using the `mcconfig` tool.

2. Files are linked and assembled into a .hex, and further assembled into a .uf2 file to be copied to the Moddable Four device.

3. xsbug and serial2xsbug are started

4. The device reboots and attaches to xsbug

## Getting Started

### MacOS host environment setup

Set up the MacOS build environment as described in the [Moddable SDK - Getting Started][modStart] guide.

Moddable is currently using the Nordic SDK v15.3.0. Documentation can be found here: 
[nrf5 SDK v15.3.0][nrf5sdkdoc]

We will use a directory named `nrf5` in your home directory to hold the SDK and various tools:

```text
cd $HOME
mkdir nrf5
cd nrf5
```

#### Install a gcc toolchain

We will use the `gnu` tools to build applications, so we will need an arm complier. Currently supported version is `8.2.1` but others may work.

The following environment variables can be modified to suit your environment.

Variable | Default | Description 
--- | ------- | -----
`NRF52_GCC_ROOT` | `$HOME/nrf5/gcc-arm-none-eabi-8-2018-q4-major` | root of the gcc toolchain. Should include `arm-non-eabi`, `bin`, etc.
`GNU_VERSION` | `8.2.1` | version of the gcc toolchain

Download from the [gcc toolchain][gcctoolchain] site.

You can set the `NRF_GCC_ROOT` to an existing installation if you already have a gcc arm compiler installed.

#### Install the nRF Command Line Tools

The nRF Command Line Tools are used in programming the Moddable Four and other nRF52840 devices.

Download and install **nRF util** and **nRF Py nrfjprog** from [Nordic Development Tools][nrfcmdtools]. Put them into a `$HOME/nRF5` directory making:

```text
...
/home/<user>/nRF5/nrfjprog/
/home/<user>/nRF5/mergehex/
...
```

Add them to your `PATH` environment variable:

```text
export PATH=$HOME/nRF5/nrfjprog:$HOME/nRF5/mergehex:$PATH
```

#### Get uf2conv.py

The Moddable Four uses a modified `Adafruit_nRF52_Bootloader`. `uf2conv.py` is a tool from Microsoft that packages up the final binary for transfer to the device.

[uf2convdownload]:https://github.com/microsoft/uf2/blob/master/utils/uf2conv.py

[Download uf2conv.py][uf2convdownload] into the `$HOME/nRF5` directory.


### nRF5 SDK Setup

#### Install the nRF5 SDK v15.3.0

Download the [Nordic nRF5 SDK v15.3.0][nrf5sdkdownload]. Select v15.3.0, and select the S140 Bluetooth 5 SoftDevice (scroll down the page).

Decompress the files into a `~/nRF5` directory making:

```text
...
/home/<user>/nRF5/nRF5_SDK_15.3.0_59ac345
/home/<user>/nRF5/s132_nrf52_6.1.1_softdevice.hex
/home/<user>/nRF5/s132_...
...
```

Make a symbolic link to the long name of the SDK. This will also allow us to change the version of the SDK without having to change many PATHs in our project.

```text
cd $NRF_SDK_DIR
ln -s nRF5_SDK_15.3.0_59ac345 nRF5_SDK
export NRF_SDK_DIR=$HOME/nRF5/nRF5_SDK
```

You can locate the SDK in a different location by setting a `NRF_SDK_DIR` environment variable:

```text
export NRF_SDK_DIR=another_location/nRF5_SDK_15.3.0_59ac345
```

#### Add a board definition

Next, we will add the Moddable Four board definition to the Nordic SDK. This provides some pin layout and constant definitions suitable for development with the Moddable Four.

The file `moddable_four.h` is found in `$MODDABLE/build/devices/nrf52/config/moddable_four.h`. Copy the file to the Nordic SDK's `components/boards/` directory:

```text
cp $MODDABLE/build/devices/nrf52/config/moddable_four.h $NRF_SDK_DIR/components/boards/
```

You'll also need to modify `components/boards/boards.h`, adding the following before `#elif defined(BOARD_CUSTOM)`:

```c
#elif defined (BOARD_MODDABLE_FOUR)
  #include "moddable_four.h"
```


## MacOS build and Deploy


### Build Moddable tools for MacOS

Build Moddable tools for MacOS (if you haven't already):

```text
cd $MODDABLE/build/makefiles/mac
make
```

### Moddable Four Programming mode

The Moddable Four has a bootloader that will automatically launch the application that is installed onto it.

To put new software onto the Moddable Four, you must put it into **programming mode**. Programming mode is indicated by the LED indicator staying lit at boot time, and a disk named `MODDABLE4` will appear on your desktop.

![Moddable Four disk](./assets/M4-disk.png#smallFramed)

Programming mode is set by double-tapping the reset button, or holding the boot button while tapping reset. The LED will light.

Your Moddable Four is now ready to be programmed.

> Note: If you do not program your device within a short period, it will reboot to the installed application.

### Build and deploy a Moddable app

Put the Moddable Four into **programming mode** and build the Moddable `helloworld` app:

```text
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p nrf52/moddable_four
```

The `mcconfig` tool builds the application, uploads the application to the Moddable Four device and launches the `xsbug` debugger.

Additional `mcconfig` features are provided by the `-t` option:

Target | Description
-------|------
`-t clean` | removes the `bin` and `tmp` directories for this application leaving libraries and other apps
`-t allclean` | removes all Moddable nrf52 applications and libraries tmp and bin
`-t startDebugger` | starts `xsbug` and `serial2xsbug`


### Debugging

A Moddable application has _native_ portions, written in C and _script_ portions written in ECMAScript.

You can debug _script_ code using `xsbug` and a serial interface with the Moddable Four.

You can debug _native_ code on the nRF52840 DK with SEGGER Embedded Studio. Examine registers, set breakpoints, view variables, memory and registers. This document does not describe that use.

<!-- MDK - some day without the FTDI dongle -->

#### Wiring the serial interface

 - Connect RX on the serial interface to `P0.03`.
 - Connect TX on the serial interface to `P0.04`.
 - Connect GND on the serial interface to a GND on the device.

**Moddable Four debugger serial connection**

![Moddable Four FTDI](./assets/M4-R0.7-SerialDebug.png#smallFramed)

Identify the serial port that the interface is using by looking for the device files matching `/dev/cu.*` before and after plugging in the serial interface.

![Identify FTDI port](./assets/identifyFTDI.png)

In this case, it is `/dev/cu.usbserial-AL035YB2`.

Set the environment variable **DEBUGGER_PORT**:

```text
export DEBUGGER_PORT=/dev/cu.usbserial-AL035YB2
```

#### Start xsbug

If you use the build target `-t copyToM4`, the build process will automatically start **xsbug** and **serial2xsbug** to connect to the debugger.

```text
cd ../path/to/application
mcconfig -d -m -p nrf52/moddable_four -t copyToM4
```

If the Moddable Four already has an application installed and you would like to debug it without reinstalling, you can use the `-t startDebugger` build target: 

```text
cd ../path/to/application
mcconfig -d -m -p nrf52/moddable_four -t startDebugger
```

> `xsbug` is used to debug the ECMAScript side of your application. `serial2xsbug` provides a serial to network socket bridge between the serial port and the `xsbug` debugger. 
     
### Native debugging on Moddable Four
_needs an update to the new board layout_

Debugging native code on the Moddable Four with the Segger Embedded Studio requires a Nordic nRF52840 dk board and this additional wiring:

![nRF52840dk to Moddable 4 connection](./assets/nrf52840dk-m4-connect.png#smallFramed) ![Moddable 4 to dk connection](./assets/m4-dk-connect.png#smallFramed)

1) dk SWD CLK to M4 SDWCLK<br>
2) dk SWD IO to M4 SWDIO<br>
3) dk RESET to M4 Reset<br>
4) dk GND DETECT to M4 GND<br>
5) dk VTG to M4 3.3v<br>

Connect Moddable 4 to a USB-to-serial dongle:

6) M4 P0.30 to FTDI Rx<br>
7) M4 P0.31 to FTDI Tx<br>


FTDI GND to GND

J-Link:Target Interface Type -> SWD

### Bootloader

Moddable Four uses a slightly modified [Adafruit nRF52 Bootloader][adafruitbootloader]. It is pre-installed and allows software and bootloader updates. There is no need to build the bootloader, as a pre-built version of the bootloader is included on the device and in the source tree.

> Note: It is very unlikely you will need to build a bootloader and you can brick your device.

For reference:

Get the bootloader sources, enter the repository directory and update the submodules:

```	
	git clone https://github.com/adafruit/Adafruit_nRF52_Bootloader.git
	cd Adafruit_nRF52_Bootloader
	git submodule update --init --recursive
```

From the repository directory add the Moddable Four configuration:

```
	cp -r $MODDABLE/build/devices/nrf52/config/bootloader/moddable_four src/boards
```

Build the bootloader and combine with the softdevice:

```
	make BOARD=moddable_four all combinehex
```

With the board hooked up to a DK through SWD interface, flash to a Moddable Four:

```
	make BOARD=moddable_four flash
```

Remove the board from the programmer and it is ready for use.


### Notes and troubleshooting

* if double-tapping the reset button doesn't put M4 into debug mode, try holding the **boot** button while pressing reset.

## Example apps with nRF52 support

App | Feature 
--- | ------- 
[helloworld](https://github.com/Moddable-OpenSource/moddable/tree/public/examples/helloworld) | xsbug 
[balls](https://github.com/Moddable-OpenSource/moddable/tree/public/examples/piu/balls) | balls using the Sharp Mirror Display ls013b4dn04
[transitions] | dk - piu on ls013b4dn04
[button] | digital in
[blink] | digital out
[TMP102] | i2c
[pulsecount] | m4 - rotary encoder
 

### Reference Documentation

Documentation for the nRF5 device and SDK can be found on the [Nordic Semiconductor Infocenter][nordicinfocenter]. You will need to be logged in to access these files.

Of particular interest and referenced earlier in this document are:

[nRF 5 SDK Documentation][nrf5sdkdoc]

### Reference Sites

[Nordic Semiconductor Infocenter][nordicinfocenter]

[SEGGER Embedded Studio Downloads][sesdownload]

[SEGGER J-Link Downloads][jlinkdownload]

[nRF5 SDK versions][nrf5sdkdownload]

[Nordic Command Line Tools][nrfcmdtools]

[Moddable Getting Started Guide][modstart]

[modstart]: https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md
[sesdownload]:https://www.segger.com/downloads/embedded-studio
[nordicinfocenter]:https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html
[nrf5sdkdoc]:https://infocenter.nordicsemi.com/topic/struct_sdk/struct/sdk_nrf5_latest.html
[nrf5sdkdownload]:https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download
[nrfcmdtools]:https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools
[jlinkdownload]:https://www.segger.com/downloads/jlink/

[gcctoolchain]:https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
[adafruitbootloader]:https://github.com/adafruit/Adafruit_nRF52_Bootloader