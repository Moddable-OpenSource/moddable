<style>
	img[src*="#smallFramed"] { width:300px;border:1px solid black; }
	img[src*="#medFramed"] { width:400px;border:1px solid black; }
</style>

# NRF52

## About This Document

This document describes how to set up and build Moddable applications for the Nordic nRF52 series processors.

## Table of Contents

* [About nRF52](#about-nrf52)
* [Development workflow](#development-workflow)
* [Getting Started](#getting-started)
  * [MacOS host environment setup](#macos-host-environment-setup)
* [nRF5 SDK Setup](#nrf5-sdk-setup)
* [Set up the nRF5 host application](#set-up-the-nrf5-host-application)
* [MacOS Build and Deploy](#macos-build-and-deploy)
  * [Build Moddable tools for MacOS](#build-moddable-tools-for-macos)
  * [Build Moddable app](#build-moddable-tools-for-macos)
  * [Build the nRF5 host application](#build-the-nrf5-host-application)
  * [Flash to device](#flash-to-device)
* [Debugging](#debugging)
  * [Wiring the serial interface](#wiring-the-serial-interface)
  * [Start xsbug](#start-xsbug)
  * [Run the application](#run-the-application) 
* [Notes and troubleshooting](#notes-and-troubleshooting)
* [Example apps with nrf52 support](#example-apps-with-qca4020-support)
* [Reference Documentation](#reference-documentation)
* [Reference Sites](#reference-sites)


## About nRF52

<!-- MDK needs refocus on Moddable Four as the development kit -->

The Nordic nRF52 is an low-powered ARM-based SoC with Bluetooth. The Moddable SDK has been ported to the [nRF52840 DK](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK)

Moddable supports FreeRTOS on the nRF52 using a MacOS based host build platform.

The build platform `-p nrf52/dk` is used to target the **nRF52840-DK** development board powered by the nRF52840.

The **Moddable Four** is based on the nRF52840. Use the build platform `-p nrf52/moddable_four`.

## Development workflow

<!-- MDK eventually, we'd like to get to a command line build & deploy like we do with Moddable Zero, One, Two and Three -->

After the initial build host setup, there are three major steps to build and deploy a Moddable application for the nRF52.

1. The application, assets, and modules are built using the `mcconfig` tool. This produces a number of `.c.o` files and an index.

2. Build the xs runtime, nRF52 libraries and small launcher application using the SEGGER Embedded Studio (SES). This will link the files generated in step 1.

3. Flash the application to the board and debug. Use SES to launch and debug the native portion of your application. Use `xsbug` to debug your ECMAScript application. 

## Getting Started

### MacOS host environment setup

Set up the MacOS build environment as described in the [Moddable SDK - Getting Started][modStart] guide.

[nrf5 SDK v15.3.0 documentation][nrf5sdkdoc]

#### Install SEGGER Embedded Studio (SES)

Download and install [SEGGER Embedded Studio][sesdownload]. Choose the version **Embedded Studio for ARM, macOS V4.22**.

Start and register your IDE. Use the menu item Tools->Package Manager and install the support package for **Nordic** devices.

#### Install SEGGER J-Link

Download and install [J-Link Software and Documentation pack for macOS][jlinkdownload]

#### Install the nRF Command Line Tools

The nRF Command Line Tools are used

Download and install `nRF util` and `nRF Py nrfjprog` from [Nordic Development Tools][nrfcmdtools]. Put them into a `~/nRF5` directory making:

```text
...
/home/<user>/nRF5/nrfjprog/
/home/<user>/nRF5/mergehex/
...
```

Add them to your `PATH` environment variable:

```text
export PATH=~/nRF5/nrfjprog:~/nRF5/mergehex:$PATH
```

#### Get uf2conv.py

For Moddable Four, we are using the Sparkfun bootloader. `uf2conv.py` is a tool from Microsoft that packages up our final binary for transfer to the device.

[uf2convdownload]:https://github.com/microsoft/uf2/blob/master/utils/uf2conv.py

[Download uf2conv.py][uf2convdownload] into the `~/nRF5` directory.

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
cd ~/nRF5
ln -s nRF5_SDK_15.3.0_59ac345 nRF5_SDK
```

The nRF build environment requires a particular location of the **$MODDABLE** directory with respect to the SDK.

From the nRF5 directory, make a symbolic link to your `$MODDABLE` directory:

```text
cd ~/nRF5
ln -s $MODDABLE ./moddable
```

### Set up the nRF5 host application

Open the `xsproj` project in SES. The location of the project is a bit deep in the tree: `$MODDABLE/build/devices/nrf52/xsProj/pca10056/s140/ses/xsproj.emProject`

Right-click on the **Project 'xsproj'** item in the left column to open the project options dialog:

![Choose project options](./assets/selectProjectOptions.png#medFramed)

In the **Project 'xsproj' Options** dialog, two items need to be changed each time you change the application you are building.

(1) Select the **Show Modified Options Only** item at the top-right to simplify finding the two items we need to change:

![Change project paths](./assets/projectPathsChangeDetail.png#medFramed)

Scroll down to the option **Additional Linker Options From File**.

(2) Set the _full path_ to the linker include file at `$MODDABLE/build/bin/nrf52/<subplatform>/debug/<application>/xs_nrf52.ind`

For example, the user _**bob**_ building **helloworld** for the **moddable_four** would use the path:

`/Users/bob/nRF5/moddable/build/bin/nrf52/moddable_four/debug/helloworld/xs_nrf52.ind`

(3) Change the _relative path_ of the **User Include Directories** to reflect the platform and application you are developing.

For example, building **helloworld** for the **moddable_four** would use the path:

`../../../../../../../build/tmp/nrf52/moddable_four/debug/helloworld`

> That's seven (7) `../`

These two items will need to change whenever you work on a new application or new device subplatform.

> Note: If you can't find the **Additional Linker Options From File** and  **User Include Directories** options, make sure you are looking at the **Project 'xsproj' Options** and not the **Solution 'xsproj' Options**.

![Project vs. Solution](./assets/projectVSsolution.png#medFramed)

## MacOS build and Deploy


### Build Moddable tools for MacOS

Build Moddable tools for MacOS (if you haven't already):

```text
cd $MODDABLE/build/makefiles/mac
make
```

### Build Moddable app

Build the Moddable `helloworld` app:

```text
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p nrf52/moddable_four
```

> The `mcconfig` tool compiles the application source code and any assets. 


### Build the nRF5 host application

In _SEGGER Embedded Studio_, ensure that the `xsproj` project is open.

> The location of the project is a bit deep in the tree: `$MODDABLE/build/devices/nrf52/xsProj/pca10056/s140/ses/xsproj.emProject`

Make sure the application paths are set up as described in the section _**Set up the nRF5 host application**_ above.

Use the menu option **Build xsproj** to build the files.

![Build and Debug](./assets/buildAndDebug.png#medFramed)

Alternately, use **Build and Debug** or **Build and Run** to both build and flash to the device.

> If there is a build error it will be noted by **Build failed** at the bottom left of the window.

![Build error](./assets/buildError.png#medFramed)

### Flash to device

The development board should be connected and powered on. Connections for peripherals should already be made (_see the section on **Debugging** below_).

Use the menu item **Build and Debug** or **Build and Run** to both build and flash to the device.

_need to document the following:_

 -	_Using nrfprog_
 - 	_Using uf2conv.py_
 - 	_Using drag/copy method_
 -  

### Debugging

A Moddable application has _native_ portions, written in C and _script_ portions written in ECMAScript.

You can debug _native_ code on the nRF52840 DK with SEGGER Embedded Studio. Examine registers, set breakpoints, view variables, memory and registers.

You can debug _script_ code using `xsbug` and a serial interface.

<!-- MDK - some day without the FTDI dongle -->

#### Wiring the serial interface

 - Connect TX on the serial interface to `P0.31`.
 - Connect RX on the serial interface to `P0.30`.
 - Connect GND on the serial interface to a GND on the device.

For the **Moddable Four** and **nRF52840 DK**

![Moddable Four FTDI](./assets/xsbugM4.png#smallFramed) ![Moddable Four FTDI](./assets/xsbugDK.png#smallFramed)

Identify the serial port that the interface is using by looking for the device files matching `/dev/cu.*` before and after plugging in the serial interface.

![Identify FTDI port](./assets/identifyFTDI.png)

In this case, it is `/dev/cu.usbserial-AL035YB2`.

#### Start xsbug

In a terminal window, launch `xsbug` and `serial2xsbug`. Use the serial port that you identified above.

```text
xsbug &
serial2xsbug /dev/cu.usbserial-AL035YB2 115200 8N1
```

> `xsbug` is used to debug the ECMAScript side of your application. `serial2xsbug` provides a serial to network socket bridge between the serial port and the `xsbug` debugger.
     
#### Run the application

In SES, use the menu item **Build and Debug** to both build and flash to the device. The window will change to look something like the following image. Items to note:

1. Application is stopped at the start of `main()`
2. Start, stop and restart buttons.
3. Step into, step over, step out and run to cursor.

![SES Debugging](./assets/sesDebug.png#medFramed)

Press the _Start_ button to start the program.

`xsbug` will show the connection is made and the application will stop at the `debugger` line.

![xsbug running helloworld](./assets/xsbugHelloworld.png#medFramed)

You can now run and debug your application.

> Note: if you have a debug build, your application will try to connect to `xsbug` at startup for a brief period. If it cannot connect, it will proceed to launch.

### Native debugging on Moddable Four

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

### Notes and troubleshooting

* sometimes xsbug doesn't connect on app start. Press the restart button in SES or the reset button on the board.
* `xsbug` needs to be running before `serial2xsbug` is started.



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

