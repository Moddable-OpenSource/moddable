# XS6 for Zephyr RTOS
Copyright 2017 Moddable Tech, Inc.

# Getting Started
Tested under follwoing environments.

* Host: Ubuntu 16.04 LTS
* Host: macOS 10.12 (Sierra)
* Zephyr: v1.8.0 (fcb9144dca68f98d24caf39532595555355c3c1d)
* Boards:
  * Arduino Due (Atmel SAM3X8E)
  * FRDM-K64F (NXP Kinetis K64F)
  * Hexiwear (NXP Kinetis K64F)
  * nRF52840 PDK (Nordic nRF52840)

## Setup Zephyr
Clone Zephyr project from the forked repository that includes all patches needs to build with Moddable.

```
$ git clone https://github.com/FantomJAC/zephyr.git -b v1.8.0-moddable
```

### For Linux Host
We use Zephyr SDK as toolchain. Follow the instruction *Development Environment Setup on Linux* on [this page](https://www.zephyrproject.org/doc/getting_started/installation_linux.html) to setup Zephyr SDK. 

***TODO: DTC and PyYAML for v1.7+***

Before proceed to the next step, make sure your environment is setup as described in *Building Sample Application* section.

```
$ export ZEPHYR_GCC_VARIANT=zephyr
$ export ZEPHYR_SDK_INSTALL_DIR=<sdk installation directory>
$ source <zephyr-project directory>/zephyr-env.sh
```

### For macOS Host
We use [GNU ARM Embedded Toolchain](https://launchpad.net/gcc-arm-embedded). Download macOS binary package, then extract it in your local directory.

```
$ tar xfz gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2 -C ~/
```

Install DTC via Homebrew.

```
$ brew install dtc
```

Install PyYAML via pip.

```
$ pip install pyyaml
```

Add symlink for Python 2.7

```
$ ln -s /usr/bin/python2.7 /usr/local/bin/python2
```

Before proceed to the next step, make sure your environment is setup.

```
$ export DTC=/usr/local/bin/dtc
$ export GCCARMEMB_TOOLCHAIN_PATH=${HOME}/gcc-arm-none-eabi-5_4-2016q3
$ export ZEPHYR_GCC_VARIANT=gccarmemb
$ source <zephyr-project directory>/zephyr-env.sh
```

## Build
Make sure your Moddable SDK environment is setup prperly. (XS6 tools must be built first.)

```
$ export MODDABLE=<moddable sdk directory>
$ export XS6_TOOL_DIR=${MODDABLE}/build/bin/mac/release
$ export PATH=${XS6_TOOL_DIR}:${PATH}
```

Then build the application. Note that the default build target is FRDM-K64F, i.e. BOARD=frdm_k64f

```
$ cd ${MODDABLE}/examples/timers
$ BOARD=frdm_k64f mcconfig -d -m -p zephyr
```

Output binaries are located under `${MODDABLE}/build/bin/zephyr` directory. `zephyr.bin` is a binary for flash (typically via dfu-util), and `zephyr.elf` is a binary for debugging purpose (e.g. GDB debugging via JLink).

## Flash
Follow the same step as described in Zephyr project wiki.

* FRDM-K64F: [Programming Flash with J-link](https://wiki.zephyrproject.org/view/NXP_FRDM-K64F#Programming_Flash_with_J-link)
* Arduino Due: [Flashing Arduino Due for Zephyr](https://wiki.zephyrproject.org/view/Arduino_Due#Flashing_Arduino_Due_for_Zephyr)

For Arduino Due, console outputs (as well as trace) will be routed to UART0 (Pin 0 and Pin 1).

For FRDM-K64F and nRF52840 PDK, console outputs will be routed to OpenSDA usb connection serial. (On Linux hosts, it will be shown as ttyACMx)

## Debugging
Debugging can be done by using Eclipse and JLink GDB server. Detailed step-by-step tutorial can be found on [this page](https://wiki.zephyrproject.org/index.php?title=NXP_FRDM-K64F&oldid=1378).

Although Zephyr project wiki doesn't mention the procedure for Arduino Due, the same procedure can be applied to Arduino Due as well. (You will need to change the *Device name* to ATSAM3X8E.)

## Zephyr Modules
*Zephyr Modules* is a collection of xs6 modules for interfacing Zephyr API features such as hardware (GPIO, I2C, SPI...), network protocols (TCP/IP, Bluetooth...) and File Systems.

```
$ git clone http://robotranch.org:3000/suchida/zephyr-modules.git
```

To build the examples, following environment should be set.

```
$ export ZEPHYR_MODULES=<zephyr-modules directory>
```

### Blink (GPIO Example)
This project shows an example usage of GPIO module.

```
$ cd ${ZEPHYR_MODULES}/examples/blink
$ mcconfig -d -m -p zephyr
```
