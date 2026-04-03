# Using the Moddable SDK with Zephyr

Copyright 2025-2026 Moddable Tech, Inc.<BR>
Updated: April 2, 2026

This document is a guide to building Moddable apps with the [Zephyr Project SDK](https://github.com/zephyrproject-rtos/zephyr/).

## Table of Contents

* [Overview](#overview)
* [Platforms](#platforms)
	* [zephyr](#platforms-zephyr)
* [Build Types](#builds)
	* [Debug](#build-debug)
	* [Instrumented](#build-instrumented)
	* [Release](#build-release)
* Setup instructions
*
    | [![Apple logo](./../assets/moddable/mac-logo.png)](#mac) | [![Windows logo](./../assets/moddable/win-logo.png)](#win) | [![Linux logo](./../assets/moddable/lin-logo.png)](#lin) |
    | :--- | :--- | :--- |
    | •  [Installing](#mac-instructions)<BR>•  [Troubleshooting](#mac-troubleshooting) | •  [Installing](#win-instructions)<BR>•  [Troubleshooting](#win-troubleshooting) | •  [Installing](#lin-instructions)<BR>•  [Troubleshooting](#lin-troubleshooting)

* [Using Zephyr Device Tree in JavaScript](#devicetree)
* [Debugging Native Code](#debugging-native-code)
* [Adding a new board](#new-board)
* [IO Connfiguration](#io-config)
* [Patches](#patches)

<a id="overview"></a>
## Overview

Before you can build applications, you need to:

- Install the Moddable SDK and build its tools
- Install the Zephyr SDK and Tools

The instructions below  verify your setup by running the `helloworld` example on your device using `mcconfig`, the command line tool to build and run applications using the Moddable SDK.

> See the [Tools documentation](./../tools/tools.md) for more information about `mcconfig`


To build for a Zephyr board, run `mcconfig` with `zephyr/<board>` for the **platform identifier**. For example, to build for the ST Nucleo F413ZH:

```sh
mcconfig -d -m -p zephyr/nucleo_f413zh
```

Zephyr supports many boards. To add Moddable support for a board that has not yet been tested, see [Adding a new board](#new-board).

You may also need to install tools for your specific target device. For example, devices from STMicroelectronics require the [STM32CubeProgrammer software](https://www.st.com/en/development-tools/stm32cubeprog.html).

<a id="builds"></a>
## Build Types
The Moddable SDK on Zephyr supports three kinds of builds: debug, instrumented, and release. Each is appropriate for a different stage in the product development process. Select which kind of build you want from the command line when running `mcconfig`.

<a id="build-debug"></a>
### Debug
A debug build is used for debugging JavaScript. In a debug build, the device attempts to connect to xsbug at startup over USB or serial depending on the device configuration. Symbols will be included for native gdb debugging.

The `-d` option on the `mcconfig` command line selects a debug build.

<a id="build-instrumented"></a>
### Instrumented
An instrumented build is used for debugging native code. In an instrumented build, the JavaScript debugger is disabled. The instrumentation data displayed graphically in xsbug is instead output to the serial console once a second.

The `-i` option on the `mcconfig` command line selects an instrumented build.

<a id="build-release"></a>
### Release
A release build is for production. In a release build, the JavaScript debugger is disabled, instrumentation statistics are not collected, and serial console output is not used.

Omitting both the `-d` and `-i` options on the `mcconfig` command line selects a release. Note that `-r` specifies display rotation rather than selecting a release build.


<a id="setup"></a>
<a id="mac"></a>
## macOS

The Moddable SDK build uses Zephyr SDK v4.3 or later.

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

9. Install Python dependencies using `west packages`.

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

#### Serial / USB Port
Different silicon families connect in different ways. For many ST boards, for example, the Moddable SDK build can automatically detect the correct port. For ESP boards, you specify the serial port by setting the `DEBUGGER_PORT` environment variable:

```sh
DEBUGGER_PORT=/dev/tty.usbserial-1410 mcconfig -d -m -p zephyr/esp32_ethernet_kit
```

Other silicon families may have different requirements.

<a id="win"></a>
## Windows

<a id="win-instructions"></a>
### Installing

1. Install the Moddable SDK tools by following the instructions in the [Getting Started document](./../Moddable%20SDK%20-%20Getting%20Started.md).

2. Install Zephyr for Windows by following the instructions in the Zephyr [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html). A summary is presented below:

	> Note: Moddable tools use the CMD shell. When installing Zephyr from the instructions, choose the `Batchfile` instructions.

3. Install Zephyr requirements:

	```sh
	winget install Kitware.CMake Ninja-build.Ninja oss-winget.gperf Python.Python.3.12 Git.Git oss-winget.dtc wget 7zip.7zip
	```

4. Create an `zephyrproject` directory in your home directory at `%USERPROFILE%\zephyrproject ` for required third party SDKs and tools.

	```sh
	cd %USERPROFILE%
	mkdir zephyrproject
	cd zephyrproject
	```

5. Create a new virtual environment (one time):

	```sh
	python -m venv .venv
	```

6. Activate the virtual environment (for each new shell):

	```sh
	zephyrproject\.venv\Scripts\activate.bat
	```

	> Note: You can deactivate the virtual environment by typing `deactivate` in your shell.

7. Install the `west` tool:

	```sh
	pip install west
	```

8. Get the Zephyr source code

	```sh
	cd %USERPROFILE%
	west init zephyrproject
	cd zephyrproject
	west update
	```

9. The Zephyr west extension command, west packages can be used to install Python dependencies.

	```sh
	cmd /c zephyr\scripts\utils\west-packages-pip-install.cmd
	```

10. Install the Zephyr SDK

	```sh
	cd %USERPROFILE%\zephyrproject\zephyr
	west sdk install
	```

11. Set the environment variables `ZEPHYR_BASE` and `DEBUGGER_PORT` using the instructions below:

	Open the "Environment Variables" dialog of the Control Panel app by following [these instructions](https://www.architectryan.com/2018/08/31/how-to-change-environment-variables-on-windows-10/). From that dialog:
	- Create a User Variable called `ZEPHYR_BASE` and set it to %USERPROFILE%\zephyrproject\zephyr
		- Variable name: `ZEPHYR_BASE`
		- Variable value (Use the "Browse Directory..." button to make this selection): `C:\Users\<user>\zephyrproject\zephyr`

    - Create a User Variable called `DEBUGGER_PORT`: the COM port for your device, e.g. `COM3`. This is used to connect your device to the `xsbug` JavaScript debugger.

	To identify the correct serial port, launch the Windows Device Manager. Open the "Ports (COM & LPT)" section, verify the Serial port adapter is displayed, and note the associated COM port (e.g. COM3).

11. Verify Zephyr SDK installation by building the `blinky` sample for your board.

	```sh
	cd %USERPROFILE%\zephyrproject\zephyr
	west build -p always -b nucleo_f413zh samples/basic/blinky
	```

	You can then flash the software to run it.

	```sh
	west flash
	```

12. Verify the complete setup by building `helloworld` for your device target:

    ```sh
    cd %MODDABLE%\examples\helloworld
    mcconfig -d -m -p zephyr/nucleo_f413zh
    ```

13. The device should connect to xsbug and stop at the `debugger` statement.
	> Note: Make sure you have built the Moddable tools in the Moddable Getting Started step.

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

```sh
- west flash: using runner stm32cubeprogrammer
      -------------------------------------------------------------------
                        STM32CubeProgrammer v2.20.0
      -------------------------------------------------------------------

libusb: error [get_usbfs_fd] libusb couldn't open USB device /dev/bus/usb/001/077, errno=13
libusb: error [get_usbfs_fd] libusb requires write access to USB device nodes
```

Change permissions on the device noted and try again.

```sh
sudo chmod a+rw /dev/bus/usb/001/077
```

<a id="devicetree"></a>
## Using Zephyr Device Tree in JavaScript
Most embedded operating systems start with C code. The header files define how software accesses the hardware. Not so with Zephyr. Instead, Zephyr begins with a Device Tree stored in a `*.dts` file. The Device Tree uses its own file format, similar to XML, JSON, and YAML. It defines all the IO from clocks, timers, and GPIOs to Wi-Fi, sensors, and displays. The Device Tree was adapted from Linux.

Zephyr uses the Device Tree to generate extremely efficient C code. For example, all IO is accessed through static data structures created by C macros. If a project tries to use IO that doesn't exist, it won't even run - it fails to compile or link.

As powerful as the Device Tree is, it creates some challenges for a dynamic language like JavaScript. For example, JavaScript code expects to be able to discover IO capabilities at runtime by inspecting JavaScript objects. But, the capabilities depend on the development board. Fortunately, ECMA-419 has a solution for this in the opaquely named [Host Provider Instance](https://419.ecma-international.org/#-27-host-provider-instance), commonly known in JavaScript as the `device` global variable.

### Examples
For the Zephyr port, our `mcdevicetree` tool generates the JavaScript code and TypeScript declarations for the `device` global from the Device Tree. Let's look at how the Device Tree works in JavaScript. Many Zephyr development boards have a button that is mapped to the name `sw0`. Here's the JavaScript code to receive notifications when the button is pressed and released:

```js
new device.button.sw0({
	onReadable() {
		trace(`sw0 state: ${this.read()}\n`);
	}
});
```

Zephyr's [blinky sample](https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/basic/blinky/src/main.c) in JavaScript is equally straightforward:

```js
const led = new device.led.led0();
let state = 1;
Timer.repeat(() => {
	state = !state;
	led.write(state);
}, 1000);
```

The Device Tree takes care of details like the GPIO port name and pin number to use, whether the GPIO is active high or low, and whether a pull-up resistor is needed.

This simplicity extends to other kinds of IO. Here's a serial echo app (Zephyr C version [here](https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/drivers/uart/echo_bot/src/main.c)):

```js
new device.serial.usart6({
	onReadable() {
		this.write(this.read());
	}
});
```

### Determining Available IO
The names `usart6`, `sw0`, and `led0` are from the Device Tree. Both labels (e.g. `usart6`) and aliases (e.g. `sw0`) are available in JavaScript. You can look these up in the Device Tree source file like a C programmer. Or your JavaScript code can discover them. For example, to list all the serial ports available, use the standard JavaScript [`for-in`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/for...in) statement:

```js
for (const port in device.serial)
	trace(port, "\n");
```

Even better, you can display them interactively in xsbug, our JavaScript debugger, by inspecting the `device` global variable.

### Network
If your Moddable SDK project manifest includes networking support, for example by including `manifest_net.json`, `mcdevicetree` includes access to all Wi-Fi and Ethernet interfaces. These objects implement the ECMA-419 [Wi-Fi](https://419.ecma-international.org/#-17-network-interface-class-pattern-wi-fi-network-interface) and [Ethernet](https://419.ecma-international.org/#-17-network-interface-class-pattern-ethernet-network-interface) APIs.

If you are using Wi-Fi, you don't need to do anything special to connect to the network beyond providing the SSID and password for your Wi-Fi access point when building:

```sh
mcconfig -d -m -p zephyr/board_with_wifi ssid="My Wi-Fi" password="secret"
```

If you are using Ethernet, the Moddable SDK runtime automatically attempts to connect when launched, before your scripts are run.

When using Wi-Fi and Ethernet, both set the device clock via NTP immediately after connecting. This is essential for secure network communication using TLS. If your device sets the time some other way, such as a Real-Time Clock peripheral, NTP clock synchronization is not done.

### Displays
If your project uses a display, typically by including `manifest_piu.json` or `manifest_commodetto.json`, the `mcdevicetree` tool automatically creates a `Screen` global for the board's display driver, if any. This allows the Piu user interface framework and Commodetto's Poco renderer to access the screen. For boards with multiple displays, the display specified in the `chosen` section of the device tree is used.

### `flash`
To access flash memory partitions, include the ECMA-419 flash module manifest `$MODDABLE/modules/io/flash/manifest.json` in your project's manifest. That creates the `device.flash.open` function to access flash partitions by name:

```js
const partition = device.flash.open({path: "partition_name"});
```

### `file`
To access a file system, include the ECMA-419 file module manifest `$MODDABLE/modules/io/files/manifest.json` in your project's manifest. That creates the `device.files` property to access the file system. Your Zephyr Device Tree must have a valid file system defined in the Zephyr Device Tree at `root.children.fstab`.

```js
const file = device.files.openFile({path: "directory/file.txt"});
```

### `keyValue`
To access the Zephyr key-value pair store (settings), include the ECMA-419 storage module manifest `$MODDABLE/modules/io/storage/manifest.json` in your project's manifest. That creates the `device.keyValue.open` function to access the key-value store. Your Zephyr Device Tree must have a valid settings subsystem defined in the Zephyr Device Tree.

```js
const domain = device.keyValue.open({path: "wifi"});
```

### TypeScript
The `device` global is different for every Zephyr board, because every board has unique hardware capabiliites and configurations. As a result, a universal TypeScript declarations file (a `.d.ts` file) cannot precisely define the `device` global for any single build. Instead, `mcdevicetree` generates the TypeScript declarations file for the board. These declarations are automatically used when building TypeScript code for Zephyr, ensuring that your code accesses only the hardware available on your development board.

<a id="debugging-native-code"></a>
## Debugging Native Code

As with all Moddable platforms, you can debug script code using `xsbug` over the USB serial interface with the Zephyr device. For more information, see the [`xsbug` documentation](../xs/xsbug.md).

For native code source level debugging, you can use [GDB](https://www.gnu.org/software/gdb/documentation/).

> Note: These instructions are for macOS and Linux.

Use the `-t debug` target to start the debugger:

```sh
mcconfig -d -m -p zephyr/nucleo_f413zh -t debug
```

> Note: if you receive the following error, you may have to start `openocd` in another window if the gdb connection fails. We have encountered this on the Nucleo L4A6ZG board running with a macOS host.

```
:3333: Operation timed out.
You can't do that when your target is `exec'
```

Start openocd:

```sh
> openocd -s ~/zephyrproject/zephyr/boards/st/nucleo_f413zh/support
```

and try to build the `-t debug` target.

```sh
mcconfig -d -m -p zephyr/nucleo_f413zh -t debug
```

<a id="new-board"></a>
## Adding a new board

Zephyr supports hundreds of development boards. The Moddable SDK includes support for some of them.

To add new board to Moddable:

1. Select a similar board from `$MODDABLE/build/devices/zephyr/targets` and duplicate its directory.

	```sh
	cd $MODDABLE/build/devices/zephyr/targets
	cp -r nucleo_f413zh new_board
	```

2. Edit the `manifest.json` file and change the `ZEPHYR_BOARD` specifier to match the Zephyr board name (same as "new_board" above). The `ZEPHYR_BOARD` must match the name used in the Zephyr SDK for your board.

3. Use the new target:

	```sh
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p zephyr/new_board
	```

<a id="io-config"></a>
## IO Configuration

Zephyr uses .dts files files to define hardware IO configuration.
Moddable uses .overlay files specified in the `zephyrOverlay` section of `manifest.json`.

The Nordic [`nrf52840dk` device]( ../../build/devices/zephyr/targets/nrf52840dk/manifest.json) device manifest demonstrates:

```jsonc
"zephyrOverlay": [
	"./analog.overlay",
	"./pwm.overlay"
],
```

### `analog`

Use `port` and `channel` to choose which analog channel to sample from. Different devices may use a different port naming schemes.

```js
const analogInput = new device.io.Analog({
	port: "adc",
	channel: 0
});
```

### `digital`

Use an object with `port` and `pin` to describe the pin of a Digital object.

```js
const led = new device.io.Digital({
	...options,
	pin: {port: "gpioe", pin: 1},
	mode: Digital.Output,
});
```

### `pwm`

Use `port` and `channel` to specify what zephyr configuration to use. You can also specify the frequncy by the `hz` property, and resolution (in number of bits) with the `resolution` property.

```js
const led1 = new device.io.PWM({
	port: "pwm0",
	hz: "100",
	resolution: "10",
	channel: "0"
});
```

### `display`
Zephyr display configuration are found in shield definition files. For example, the `stm32u5a9j_dk` target's [`manifest.json`]( ../../build/devices/zephyr/targets/stm32u5a9j_dk/manifest.json) contains:

```jsonc
"zephyrShields": [
	"st_lcd_dsi_mb1835"
],
```

<a id="patches"></a>
## Patches

### ESP32 setjmp

On ESP32 targets, each time `setjmp` is called several lines are logged to the console. This indicates some kind of processor fault. Fortunately, the setjmp still seems to succeeed because a later longjmp works. Because the XS JavaScript engine in the Moddable SDK uses `setjmp` to implement JavaScript exceptions, this can happen often enough that execution is slowed significantly. There [does not appear](https://github.com/zephyrproject-rtos/zephyr/issues/68524) to be a simple solution. As a workaround, you can comment out most of the exception log. Unfortunately, removing all of it cuases the system to hang. The patch is in `arch/xtensa/core/vector_handlers.c` on the following lines 

```c
	case EXCCAUSE_SYSCALL:
		/* Just report it to the console for now */
		EXCEPTION_DUMP(" ** SYSCALL PS %p PC %p",
			(void *)bsa->ps, (void *)bsa->pc);
		xtensa_dump_stack(interrupted_stack);
```

Comment out the call to `xtensa_dump_stack`.

