# QCA 4020

## About This Document

This document describes how to set up and build Moddable applications for the Qualcomm QCA4020 series processors.

## Table of Contents

* [About QCA4020](#about-qca4020)
* [Development workflow](#development-workflow)
* [Getting Started](#getting-started)
  * [Linux Host environment setup](#linux-host-environment-setup)
  * [Install the QCA4020 SDK](#get-the-qca4020-sdk)
  * [Set Environment Variables](#environment-variables)
  * [ARM Toolchain setup](#arm-toolchain-setup)
* SDK Setup
  * [FreeRTOS Setup](#freertos-setup)
  * [Build qurt](#build-qurt)
  * [Build FreeRTOS](#build-freertos)
  * [Install OpenOCD](#openocd)
  * [Modify device configuration for debugging](#change-device-configuration-for-debugging)
  * [Adjust Link Map](#adjust-link-map)
  * [Stack](#stack)
* [Linux Build and Deploy](#linux-build-and-deploy)
  * [Build Moddable Tools for Linux](#build-linux-tools)
  * [Build Moddable App](#build-moddable-app)
  * [Flash to device](#flash-to-device)
* [Debugging](#debugging)
  * [Start xsbug](#start-xsbug)
  * [Launch gdb](#launch-gdb)
  * [Boot to App](#boot-to-app)
* [Notes and troubleshooting](#notes-and-troubleshooting)
* [Example apps with qca4020 support](#example-apps-with-qca4020-support)

* [Reference Documentation](#reference-documentation)


## About QCA4020

The Qualcomm QCA4020 is a processor from Qualcomm that incorporates bluetooth, wifi in a low-powered ARM based device.

You will need to have a [Qualcomm Developer Network][devNet] account to download the SDK. Moddable supports version 3.x.

Moddable uses FreeRTOS on the QCA4020. Development was done for a CDB development board using a Linux host build platform.

The platform `-p qca4020/cdb` is used to designate the CDB  development board powered by the QCA4020.


## Development workflow

After the initial setup, there are two major steps to build a Moddable application for the QCA4020.

First, the ECMAScript application, assets, modules and XS runtime are built into an archive using Moddable's `mcconfig` tool. This produces a `xs_qca4020.a` archive file.

Then, you will build the small launcher application which includes the `xs_qca4020.a` archive and the Qualcomm libraries.

Then you will flash the application to the board.

To debug, you will start `xsbug` to debug your ECMAScript application. You may also need to launch gdb to debug the native portion of your application.

## Getting Started

### Linux Host environment setup

> The Moddable SDK has been tested on 16.04 LTS (64-bit), 18.04.1 (64-bit) and Raspberry Pi Desktop (32-bit) operating systems. These setup instructions assume that a GCC toolchain has already been installed.

Set up the Linux environment as described in the [Moddable SDK - Getting Started][modStart] guide.

#### Get the QCA4020 SDK

Get the [Qualcomm QCA4020 SDK version 3.0][sdk3] from the Qualcomm Developer Network's [Tools & Resources - QCA4020][tools] page.

Decompress the SDK file into a ```~/qualcomm``` directory making

	/home/<user>/qualcomm/qca4020

It should contain the directory ```target/```.

#### Environment variables

Set some environment variables:

	export SDK=/home/<user>/qualcomm/qca4020/target
	export CHIPSET_VARIANT=qca4020
	export RTOS=freertos
	export BOARD_VARIANT=cdb
	export QCA_GCC_ROOT=/usr/local/bin/gcc-arm-none-eabi-6-2017-q2-update

Add the path: `/home/user/qualcomm/qca4020/target/bin/cortex-m4` directory to the `PATH` environment variable:

	export PATH=$PATH:$SDK/bin/cortex-m4
	
#### ARM Toolchain setup

Get version 6.2 of the GNU embedded toolchain for ARM-based processors from the [Arm Developer Site][armdevsite] 

Add the path of the toolchain binaries to the `PATH` environment variable depending on where you installed it.

	export PATH=$PATH:/usr/local/bin/gcc-arm-none-eabi-6-2017-q2-update/bin/

###### Ref: [B] Development Kit User Guide - section 3.2

#### FreeRTOS Setup

Download **FreeRTOS**:

	cd ~/qualcomm
	git clone https://source.codeaurora.org/external/quartz/FreeRTOS
	cd FreeRTOS
	git checkout v8.2.1

Dowload the **qurt** interface layer:

	cd ~/qualcomm
	git clone https://source.codeaurora.org/external/quartz/ioe/qurt

> Note: You may get an warning here. Continue.
	
	cd qurt
	git checkout v2.0
	
##### Configure FreeRTOS

The FreeRTOS configuration file can be found at

```
~/qualcomm/FreeRTOS/1.0/FreeRTOS/Demo/QUARTZ/FreeRTOSConfig.h
```

Change the following config values:

	#define configTICK_RATE_HZ        ( ( TickType_t )  1000)
	#define configTOTAL_HEAP_SIZE     ( ( size_t ) 0x13000 ) )

> Note: If you change the ```configTOTAL_HEAP_SIZE ```, you will need to change ```RTOS_HEAP_SIZE``` in the **DefaultTemplateLinkerScript.ld** as described below. You will also need to rebuild **qurt** and **FreeRTOS** and copy them into the proper place, also described below.

To assist in development, you may want to enable two additional configuration values to identify stack overflow and out of memory conditions.

To trigger `vApplicationStackOverflowHook()` in `$MODDABLE/build/devices/qca4020/xsProj/src/main.c` on a stack overflow, change the following config value:

	#define configCHECK_FOR_STACK_OVERFLOW  2

To trigger `vApplicationMallocFailedHook()` when memory allocation fails, change the following config value:
		
	#define configUSE_MALLOC_FAILED_HOOK    1

#### Build qurt

###### Ref: [B] Development Kit User Guide - section 3.8

Build the **qurt** adapter layer for FreeRTOS and copy it into place:

    cd ~/qualcomm/qurt/FreeRTOS/2.0
    make all
    cp output/qurt*.lib ~/qualcomm/qca4020/target/lib/cortex-m4IPT/freertos/

#### Build FreeRTOS

Build FreeRTOS and copy it into place:

    cd ~/qualcomm/FreeRTOS/1.0/FreeRTOS/Demo/QUARTZ
    make all
    cp output/free_rtos.lib ~/qualcomm/qca4020/target/lib/cortex-m4IPT/freertos/

### OpenOCD

###### Ref: [B] Development Kit User Guide - section 3.7.2.1

[OpenOCD](http://openocd.org) is used to flash the binary to the QCA4020. It needs to be built with the ```--enable-ftdi``` option.

[Download openocd-0.10.0](https://sourceforge.net/projects/openocd/files/openocd/0.10.0/) and build. The location doesn't matter.

	cd open-ocd-0.10.0
	./configure --enable-ftdi
	make install

> Note: If there is an error `libusb-1.x not found...` you can fix by installing `libusb-dev`.
>
	sudo apt-get install libusb-1.0-0.dev
	
#### Change device configuration for debugging

###### Ref: [B] Development Kit User Guide - section 3.7.2 (JTAG debug mode)

Make sure system sleep is disabled so that JTAG can connect on boot:

In the file `$MODDABLE/build/devices/qca4020/xsProj/src/export/DevCfg_master_devcfg_out_cdb.xml` change:

    <driver name="Sleep"> ...
        <props id="0x2" oem_configurable="false" type="0x00000002"> 1 </props>

to

		<props ... > 0 </props>


Disable watchdog so it doesn't die in `err_jettison_core` when you're doing something in `xsbug`. Same file as above:

Change:

    <props name="dog_hal_disable" type="0x00000002”> 0 </props>

to

    <props name="dog_hal_disable" type="0x00000002”> 1 </props>
    
> Note: you will want to re-enable the watchdog before shipping.


### Adjust Link Map

A link map is used to specify where in memory the different components of your application will go. 

The QCA4020 allocates RAM to read-only memory (instructions and data), and read-write memory for each of the different operating modes of the chip. Moddable apps run primarily in the Full Operating Mode (***FOM***).

The QCA4020 also has flash memory that can be used to store data and code. Moddable stores resources, XS bytecode and static variables in flash. Application object code runs out of XIP (execute in place) memory.

We will need to adjust some values in the link map to give us more data space in RAM, and to move read-only data to flash.

In the file `MODDABLE/build/devices/qca4020/xsProj/src/export/DevCfg_master_devcfg_out_cdb.xml` do the following:

Disable **DEP** (data execution prevention) by setting the property value to 0:

    <props id="1" id_name="PLATFORM DEP ENABLE" oem_configurable="true" helptext="Enable or disable data execution prevention." type="0x00000002">
       0
    </props>

Disabling DEP allows more RAM to be allocated by applications.

In the file `~/qualcomm/qca4020/target/bin/cortex-m4/freertos/DefaultTemplateLinkerScript.ld` do the following:

##### 1) Check `RTOS_HEAP_SIZE`

Make sure `RTOS_HEAP_SIZE` has the same value as the define `configTOTAL_HEAP_SIZE` in **FreeRTOSConfig.h** described above.

	RTOS_HEAP_SIZE = 0x13000;

> Note: When `configUSE_MALLOC_FAILED_HOOK` is defined in **FreeRTOSConfig.h**, the `vApplicationMallocFailedHook()` error handler is invoked. This can be used to determine if you need to allocate more memory to the FreeRTOS heap.

> `vApplicationMallocFailedHook()` is found in `$MODDABLE/build/devices/qca4020/xsProj/src/main.c`

##### 2) Move static data to Flash

Move your application's static data into flash (XIP):

    XIP_OEM_RO_REGION :
    {
        *(XIP_OEM_RO_SECTION)

After this, add:

        *(.flash*)
        *(.flash.rodata*)

##### 3) Allocate 40 KB to read-only memory and 262 KB RAM to data space 

###### Ref: 	(see similar in Document [C]: QCA402x (CDB2x) Programmers Guide - section 4.3.4 Resize application memory)

Change the `RAM_FOM_APPS_RO_MEMORY` and `RAM_FOM_APPS_DATA_MEMORY` origin and length values:

	RAM_FOM_APPS_RO_MEMORY   (RX) : ORIGIN = 0x10046000, LENGTH = 0x0a000
	RAM_FOM_APPS_DATA_MEMORY (W)  : ORIGIN = 0x10050000, LENGTH = 0x40000

### Stack

You may change the stack size of the XS task.

> Note: When `configCHECK_FOR_STACK_OVERFLOW` is set to **2** in **FreeRTOSConfig.h**, the `vApplicationStackOverflowHook()` error handler is invoked.

> `vApplicationStackOverflowHook()` is found in `$MODDABLE/build/devices/qca4020/xsProj/src/main.c`

If you're running into stack overflows, you can change the define `xsMain_THREAD_STACK_SIZE` in: `$MODDABLE/build/devices/qca4020/base/xsmain.c`.



## Linux build and Deploy

### Build Linux tools and Moddable app

Build Moddable tools for linux (if you haven't already):

	cd $MODDABLE/build/makefiles/lin
	make

Build a Moddable app:

	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p qca4020
	
> This creates a library `xs_qca4020.a` that contains the app and its assets and the xs runtime.

You will need to set some environment variables:

	export APP_NAME=helloworld
	export DEBUG=1
	
Build the stub application to link in the qca4020 libraries and *main.c*.
	
	cd $MODDABLE/build/devices/qca4020/xsProj/build/gcc
	make

### Flash to device
###### Ref: [B] Development Kit User Guide - section 3.6 (Linux Flash)

> Note: Flashing takes a long time (over 3 minutes running Ubuntu in a MacOS VirtualBox) with a _lot_ of output on the console.

#### Ensure the jumpers are configured properly:

![Jumper Diagram](assets/QCA4020CDB-SPI.png)

Make sure jumper is on J31 1&2 and reset the board. If J31 is removed, the board will boot directly into the app.

    Pin Configuration for JTAG GPIO 53:50

    To use JTAG3 (which doesn't conflict with SPI):

    // JTAG bootstrap
    J30 Connect pins 2 and 3 for JTAG    (GPIO_25 low)
    J32 Connect pins 1 and 2 for JTAG    (GPIO_18 high)

    // JTAG 
    J37 pin 2 Connect J5 pin 28 (JTAG TCK)
    J38 pin 2 Connect J5 pin 34 (JTAG TDI)
    J39 pin 2 Connect J5 pin 30 (JTAG TDO)
    J40 pin 2 Connect J5 pin 32 (JTAG TMS)

#### Flash to device

The development board should be connected and powered on. The lower USB port is used to connect to Linux.

> Note: When you plug in the cdb board, two USB devices will be made available to Linux: `/dev/ttyUSB0` and `/dev/ttyUSB1`. The first will be captured by OpenOCD and `/dev/ttyUSB1` will be used to connect to `xsbug`.

Go to project build directory and run the flash tool:

	cd $MODDABLE/build/devices/qca4020/xsProj/build/gcc
	sh flash_openocd.sh
	
> Note: the flash tool starts **openocd** and leaves it running. This is good as we will need it running later for gdb.


### Debugging

#### Start xsbug

In another terminal window, launch `xsbug` and `serial2xsbug`:

    xsbug &
    serial2xsbug /dev/ttyUSB1 115200 8N1

> `xsbug` is used to debug the ECMAScript side of your application. `serial2xsbug` connects the output from the qca4020 and sends it to `xsbug`.
     
#### Launch gdb

Go to the project build directory and run gdb:

    cd $MODDABLE/customers/qualcomm/baseapp/build/gcc
    arm-none-eabi-gdb -x v2/quartzcdb.gdbinit

> `gdb` is used to debug the native C portion of your application.

A considerable amount of output will appear on the screen. At the `(gdb)` prompt type `c` to continue three times to launch the app. You can also set breakpoints, view source and variables, etc.

`xsbug` will show the connection is made and the application should start.

#### Boot to App

Disconnect jumper on J31 between pins 1 & 2 and reset the board.

The board will boot into the app instead of waiting for `gdb`.

> Note: if you have a debug build, your application will still try to connect to `xsbug` at startup. If it can not connect, it will proceed to launch. You can continue to debug your application without using `gdb`.


### Notes and troubleshooting

* `xsbug` needs to be running before `serial2xsbug` is started.

* Use bash aliases or other scripts to make your life easier. For example:

	`alias killocd='killall openocd' `
	`alias doocd='killall openocd; openocd -f qca402x_openocd.cfg'`
	`alias dogdb='arm-none-eabi-gdb -x v2/quartzcdb.gdbinit'`

* You will need to restart `serial2xsbug` if you disconnect the board or shut down the virtual usb interface.

* When the device crashes and you want to restart your debug session, you will need to restart **gdb**.

   - You may also need to kill and restart openocd

    `killall openocd; openocd -f qca402x_openocd.cfg`

* Press the Reset button (near **J20**) before reflashing.

* The upper USB port near the switch is for Windows connection. Do not use it with a Linux Host.

* The lower USB port provides JTAG connection (`/dev/ttyUSB0` for JTAG and `/dev/ttyUSB1` for serial output (and `xsbug`)


## Example apps with qca4020 support

App | Feature 
--- | ------- 
helloworld | xsbug 
timers | timers
files | files
preference | preferences
networkpromises | promises
network/ble/colorific | BLE 
network/ble/colorific | BLE 
network/ble/heart-rate-monitor | BLE
network/ble/security-server |
network/ble/wifi-connection-server |
httpgetjson | http, json parsing
httpserver | http server
network/ping | 
socketreadwrite | network socket demonstration
wifiaccesspoint | configure device to act as an access point
wifiscan | scan for SSIDs
httpsgetstreaming | https
piu/balls | animation on ili9341
piu/cards | animation on ili9341
images | animation, image decompress
mini-weather | piu application to fetch weather conditions
text | text rendering

### Reference Documentation

Documentation for the QCA4020 can be found at the [Qualcomm Developer Network][devNet]. You will need to be logged in to access these files.

Of particular interest and referenced earlier in this document are:

[A] [QAPI Specification][A]

Contains the API Documentation

[B] [Development Kit User Guide][B]

Describes setting up the SDK, development environment, how to flash and debug. Also describes Board Jumper settings.

[C] [QCA402x (CDB2x) Programmers Guide][C]

Contains an overview, networking features, programming model, memory model, etc. GPIO configuration and interfaces

[A]: https://developer.qualcomm.com/download/qca4020-qca4024/qca402x-qapi-specification.pdf
[B]: https://developer.qualcomm.com/download/qca4020-qca4024/qca402x-cdb2x-development-kit-user-guide.pdf
[C]: https://developer.qualcomm.com/download/qca4020-qca4024/qca402x-cdb2x-programmers-guide.pdf
[devnet]: https://developer.qualcomm.com/hardware/qca4020-qca4024/tools-qca4020
[sdk3]:https://developer.qualcomm.com/download/qca4020-qca4024/qca4020or11-qca-oem-sdkcdb.zip
[tools]:https://developer.qualcomm.com/hardware/qca4020-qca4024/tools-qca4020
[armdevsite]:https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
[modstart]: https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md
