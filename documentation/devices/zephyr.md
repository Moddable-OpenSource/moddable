# Using the Moddable SDK with Zephyr

Copyright 2025 Moddable Tech, Inc.<BR>
Updated: October 2, 2025

This document is a guide to building apps with the Zephyr SDK.

## Table of Contents

* [Overview](#overview)
* [Platforms](#platforms)
	* [zephyr](#platforms-zephyr)
* [Build Types](#builds)
	* [Debug](#build-debug)
	* [Instrumented](#build-instrumented)
	* [Release](#build-release)
* Setup instructions

    | [![Apple logo](./../assets/moddable/mac-logo.png)](#mac) | [![Windows logo](./../assets/moddable/win-logo.png)](#win) | [![Linux logo](./../assets/moddable/lin-logo.png)](#lin) |
    | :--- | :--- | :--- |
    | •  [Installing](#mac-instructions)<BR>•  [Troubleshooting](#mac-troubleshooting) | •  [Installing](#win-instructions)<BR>•  [Troubleshooting](#win-troubleshooting) | •  [Installing](#lin-instructions)<BR>•  [Troubleshooting](#lin-troubleshooting)

* [Troubleshooting](#troubleshooting)
* [Debugging Native Code](#debugging-native-code)
* [Adding a new board](#new-board)

<a id="overview"></a>
## Overview

Before you can build applications, you need to:

- Install the Moddable SDK and build its tools
- Install the Zephyr SDK and Tools

The instructions below will have you verify your setup by running the `helloworld` example on your device using `mcconfig`, the command line tool to build and run applications using the Moddable SDK.

> See the [Tools documentation](./../tools/tools.md) for more information about `mcconfig`


To build for a Zephyr board, run `mcconfig` with `zephyr/<board>` for the **platform identifier**. For example, to build for the ST Nucleo F413ZH:

```sh
mcconfig -d -m -p zephyr/nucleo_f413zh
```

Zephyr supports many boards. To add Moddable support for a board that has not yet been tested, see [Adding a new board](#new-board).

You may also need to install tools for your specific target device. For example, devices from STMicroelectronics, you will need to download and install the [STM32CubeProgrammer software](https://www.st.com/en/development-tools/stm32cubeprog.html).

<a id="builds"></a>
## Build Types
The Zephyr supports three kinds of builds: debug, instrumented, and release. Each is appropriate for different stages in the product development process. You select which kind of build you want from the command line when running `mcconfig`.

<a id="build-debug"></a>
### Debug
A debug build is used for debugging JavaScript. In a debug build, the device will attempt to connect to xsbug at startup over USB or serial depending on the device configuration. Symbols will be included for native gdb debugging.

The `-d` option on the `mcconfig` command line selects a debug build.

<a id="build-instrumented"></a>
### Instrumented
An instrumented build is used for debugging native code. In an instrumented build, the JavaScript debugger is disabled. The instrumentation data usually available in xsbug is output to the serial console once a second.

The `-i` option on the `mcconfig` command line selects an instrumented build.

<a id="build-release"></a>
### Release
A release build is for production. In a release build, the JavaScript debugger is disabled, instrumentation statistics are not collected, and serial console output is suppressed.

Omitting both the `-d` and `-i` options on the `mcconfig` command line selects a release. Note that `-r` specifies display rotation rather than selecting a release build.


<a id="setup"></a>
<a id="mac"></a>
## macOS

The Moddable SDK build for Zephyr currently uses the Zephyr SDK v4.2.0-rc1 (commit `ffb28eed`).

<a id="mac-instructions"></a>
### Installing

1. Install the Moddable SDK tools by following the instructions in the [Getting Started document](./../Moddable%20SDK%20-%20Getting%20Started.md).

2. Create an `zephyrproject` directory in your home directory at `~/zephyrproject ` for required third party SDKs and tools.

	```sh
	mkdir ~/zephyrproject
	cd ~/zephyrproject
	```

3. Set the environment `ZEPHYR_BASE`

	```sh
	export ZEPHYR_BASE=~/zephyrproject/zephyr
	```

3. If you use macOS Catalina (version 10.15) or later, add an exemption to allow Terminal (or your alternate terminal application of choice) to run software locally that does not meet the system's security policy. Without this setting, the precompiled GNU Arm Embedded Toolchain downloaded in the next step will not be permitted to run.

    To set the security policy exemption for Terminal, go to Security & Privacy System Preferences, select the Privacy tab, choose Developer Tools from the list on the left, and then tick the checkbox for Terminal or the alternate terminal application from which you will be building Moddable SDK apps. The end result should look like this:

    ![Catalina Developer Options](../assets/getting-started/catalina-security.png)

4. Install Zephyr requirements:

	```sh
	brew install cmake ninja gperf python3 python-tk ccache qemu dtc libmagic wget openocd
	```

5. Create a new virtual environment (one time):

	```sh
	python3 -m venv ~/zephyrproject/.venv
	```

6. Activate the virtual environment (for each new shell):

	```sh
	source ~/zephyrproject/.venv/bin/activate
	```
> Note: You can deactivate the virtual environment by typing `deactivate` in your shell.

7. Install the `west` tool:

	```sh
	pip install west
	```

8. Get the Zephyr SDK

	```sh
	west init ~/zephyrproject
	cd ~/zephyrproject
	west update
	```

9. The Zephyr west extension command, west packages can be used to install Python dependencies.

	```sh
	west packages pip --install
	```

10. Install the Zephyr SDK

	```sh
	cd ~/zephyrproject/zephyr
	west sdk install
	```

11. Verify Zephyr SDK installation by building the `blinky` sample for your board.

	```sh
	west build -p always -b nucleo_f413zh samples/basic/blinky
	```

	You can then flash the software to run it.

	```sh
	west flash
	```

12. Verify the complete setup by building `helloworld` for your device target:

    ```sh
    cd ${MODDABLE}/examples/helloworld
    mcconfig -d -m -p zephyr/nucleo_f413zh
    ```

13. The device should connect to xsbug and stop at the `debugger` statement.


<a id="mac-troubleshooting"></a>
### Troubleshooting


<a id="win"></a>
## Windows

<a id="win-instructions"></a>
### Installing

*Not yet supported*


<a id="lin"></a>
## Linux

<a id="lin-instructions"></a>
### Installing

1. Install the Moddable SDK tools by following the instructions in the [Getting Started document](./../Moddable%20SDK%20-%20Getting%20Started.md).

2. Create an `zephyrproject` directory in your home directory at `~/zephyrproject ` for required third party SDKs and tools.

	```sh
	mkdir ~/zephyrproject
	cd ~/zephyrproject
	```

3. Set the environment `ZEPHYR_BASE`

	```sh
	export ZEPHYR_BASE=~/zephyrproject/zephyr
	```

4. Install Zephyr requirements:

	```sh
	sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
	```

5. Create a new virtual environment (one time):

	```sh
	python3 -m venv ~/zephyrproject/.venv
	```

6. Activate the virtual environment (for each new shell):

	```sh
	source ~/zephyrproject/.venv/bin/activate
	```

> Note: You can deactivate the virtual environment by typing `deactivate` in your shell.

7. Install the `west` tool:

	```sh
	pip install west
	```

8. Get the Zephyr source code

	```sh
	west init ~/zephyrproject
	cd ~/zephyrproject
	west update
	```
<!--
is exporting a Zephyr CMake package necessary?
-->

9. The Zephyr west extension command, west packages can be used to install Python dependencies.

	```sh
	west packages pip --install
	```

10. Install the Zephyr SDK

	```sh
	cd ~/zephyrproject/zephyr
	west sdk install
	```

11. Verify Zephyr SDK installation by building the `blinky` sample for your board.

	```sh
	west build -p always -b nucleo_f413zh samples/basic/blinky
	```

	You can then flash the software to run it.

	```sh
	west flash
	```

12. Verify the complete setup by building `helloworld` for your device target:

    ```sh
    cd ${MODDABLE}/examples/helloworld
    mcconfig -d -m -p zephyr/nucleo_f413zh
    ```

13. The device should connect to xsbug and stop at the `debugger` statement.

<a id="lin-troubleshooting"></a>
### Troubleshooting

> Installation failure

If there is an error with libusb when trying to install, change the permissions on the device:

```
- west flash: using runner stm32cubeprogrammer
      -------------------------------------------------------------------
                        STM32CubeProgrammer v2.20.0
      -------------------------------------------------------------------

libusb: error [get_usbfs_fd] libusb couldn't open USB device /dev/bus/usb/001/077, errno=13
libusb: error [get_usbfs_fd] libusb requires write access to USB device nodes
```

Change permissions on the device noted and try again.

```
sudo chmod a+rw /dev/bus/usb/001/077
```

<a id="debugging-native-code"></a>
### Debugging Native Code

As with all Moddable platforms, you can debug script code using `xsbug` over the USB serial interface with the Zephyr device. For more information, see the [`xsbug` documentation](../xs/xsbug.md).

For native code source level debugging, you can use [GDB](https://www.gnu.org/software/gdb/documentation/).

Use the `-t debug` target to start the debugger:

```
mcconfig -d -m -p zephyr/nucleo_f413zh -t debug
```

> Note: if you receive this error, you may have to start `openocd` in another window if the gdb connection fails. We have encountered this on the Nucleo L4A6ZG board running with a MacOS host.

```
:3333: Operation timed out.
You can't do that when your target is `exec'
```

Start openocd:

```
> openocd -s ~/zephyrproject/zephyr/boards/st/nucleo_f413zh/support
```

and try to build the `-t debug` target.

```
mcconfig -d -m -p zephyr/nucleo_f413zh -t debug
```

<a id="new-board"></a>
# Adding a new board

Zephyr supports many boards.

To add new board to Moddable:

1. Select a similar board from `$MODDABLE/build/devices/zephyr/targets` and duplicate its directory.

	```
	cd $MODDABLE/build/devices/zephyr/targets
	cp -r nucleo_f413zh newboard
	```

2. Edit the `manifest.json` file and change the `ZEPHYR_BOARD` specifier to match the Zephyr board name.

3. Use the new target:

	```
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p zephyr/newboard
	```
