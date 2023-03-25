# Using the Moddable SDK with nRF52840

Copyright 2021-2023 Moddable Tech, Inc.
Revised: March 22, 2023

This document provides a guide to building apps for the nRF52840 SoC from Nordic with the Moddable SDK.

## Table of Contents

* [Overview](#overview)
* [Platforms](#platforms)
	* [nrf52](#platforms-nrf52)
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
* [Bootloader](#bootloader)
	* [Installing the bootloader](#install-bootloader)
	* [Updating the bootloader](#update-bootloader)
* [nRF5 SDK modifications](#nrf5-sdk-mods)

<a id="overview"></a>
## Overview

Before you can build applications, you need to:

- Install the Moddable SDK and build its tools
- Install the required drivers and development tools for the nRF52840 platform

The instructions below will have you verify your setup by running the `helloworld` example on your device using `mcconfig`, a command line tool that builds and runs Moddable applications.

> See the [Tools documentation](./../tools/tools.md) for more information about `mcconfig`


When building with `mcconfig`, you specify your device target by providing the **platform identifier** of your development board to the `-p` argument. For example, use the following command to build for Moddable Four:

```text
mcconfig -d -m -p nrf52/moddable_four
```

A list of available nRF52840 subplatforms and their platform identifiers is provided in the **Platforms** section below.

<a id="platforms"></a>
## Platforms

<a id="platforms-nrf52"></a>
### nRF52840

nRF52840 has the following features:

- 64 MHz Cortex-M4 with FPU
- BLE
- 256 KB RAM
- 1 MB Flash

| Name | Platform identifier | Key features | Links |
| :---: | :--- | :--- | :--- |
| <img src="./../assets/devices/moddable-four.png" width=125><BR>Moddable Four | `nrf52/moddable_four`<BR>`simulator/moddable_four` | - **1.28" 128x128 Monochrome**<BR>- Sharp Mirror display<BR>- BLE<BR>- Jogdial<BR>- Accelerometer<BR>- Button and LED<BR>- CR2032 coin-cell power<BR>- 12 External GPIO pins  | <li>[Moddable Four developer guide](./moddable-four.md)</li><li>[Moddable product page](https://www.moddable.com/purchase.php)</li> |
| <img src="./../assets/devices/nrf52-pca10056.png" width=125><BR>Nordic nRF52840 DK pca10056 | `nrf52/dk` | - 4 LEDs<BR>- 4 Buttons<BR>- All pins accessible<BR>- BLE<BR>- CR2032 coin-cell power  | <li>[Product page](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk)</li> |
| <img src="./../assets/devices/nrf52-sparkfun.png" width=125><BR>Sparkfun Pro nRF52840 Mini | `nrf52/sparkfun` | - 1 LED<BR>- 1 Button<BR>- BLE<BR>- JST Power connector<BR>- Qwiic connector<BR>- 17 GPIO pins  | <li>[Product page](https://www.sparkfun.com/products/15025)</li> |
| <img src="./../assets/devices/nrf52-makerdiary.png" width=125><BR>Makerdiary nRF58240 MDK | `nrf52/makerdiary` | - 1  3-color LED<BR>- 1 Button<BR>- BLE<BR>- 12 GPIO pins  | <li>[Product page](https://makerdiary.com/products/nrf52840-mdk-usb-dongle)</li> |
| <img src="./../assets/devices/nrf52-xiao.png" width=125><BR>Seeed Studio XIAO nRF52840 | `nrf52/xiao` | - 1 3-color LED<BR>- 1 Button<BR>- BLE<BR>- 11 GPIO pins  | <li>[Product page](https://www.seeedstudio.com/Seeed-XIAO-BLE-nRF52840-p-5201.html)</li> |
| <img src="./../assets/devices/nrf52-itsybitsy.png" width=125><BR>Adafruit ItsyBitsy nRF52840 Express | `nrf52/itsybitsy` | - 1 LED<BR>- 1 Button<BR>- BLE<BR>- 21 GPIO pins  | <li>[Product page](https://www.adafruit.com/product/4481)</li> |

<a id="builds"></a>
## Build Types
The nRF52 supports three kinds of builds: debug, instrumented, and release. Each is appropriate for different stages in the product development process. You select which kind of build you want from the command line.
    
<a id="build-debug"></a>
### Debug
A debug build is used for debugging JavaScript. In a debug build, the device will attempt to connect to xsbug at startup over USB or serial depending on the device configuration. Symbols will be included for native gdb debugging.

The `-d` option on the `mcconfig` command line selects a debug build.

<a id="build-instrumented"></a>
### Instrumented
A debug build is used for debugging native code. In an instrumented build, the JavaScript debugger is disabled. The instrumentation data usually available in xsbug is output to the serial console once a second.

The `-i` option on the `mcconfig` command line selects an instrumented build.

<a id="build-release"></a>
### Release
A release build is for production. In a release build, the JavaScript debugger is disabled, instrumentation statistics are not collected, and serial console output is suppressed.

Omitting the `-d` and `-i` options on the `mcconfig` command line selects a release. Note that `-r` specifies rotation rather than selecting a release build.


<a id="setup"></a>
<a id="mac"></a>
## macOS

The Moddable SDK build for nrf52 currently uses Nordic nRF5 SDK v17.0.2.

<a id="mac-instructions"></a>
### Installing

1. Install the Moddable SDK tools by following the instructions in the [Getting Started document](./../Moddable%20SDK%20-%20Getting%20Started.md).

2. Create an `nrf5` directory in your home directory at `~/nrf5 ` for required third party SDKs and tools.

3. If you use macOS Catalina (version 10.15), add an exemption to allow Terminal (or your alternate terminal application of choice) to run software locally that does not meet the system's security policy. Without this setting, the precompiled GNU Arm Embedded Toolchain you will download in the next step will not be permitted to run.

    To set the security policy exemption for Terminal, go into the Security & Privacy System Preferences, select the Privacy tab, choose Developer Tools from the list on the left, and then tick the checkbox for Terminal or the alternate terminal application from which you will be building Moddable SDK apps. The end result should look like this:

    ![Catalina Developer Options](../assets/getting-started/catalina-security.png)

4. On **x86** Mac, Download version 12.2.1 [AArch32 bare-metal target (arm-none-eabi)](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-darwin-x86_64-arm-none-eabi.tar.xz) of the GNU Arm Embedded Toolchain from the [Arm Developer](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) website. Uncompress the archive and move the `arm-gnu-toolchain-12.2.rel1-darwin-x86_64-arm-none-eabi` directory into the `nrf5` directory.

5. On **M1** Mac, Download version 12.2.1 [AArch32 bare-metal target (arm-none-eabi)](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-darwin-arm64-arm-none-eabi.tar.xz) of the GNU Arm Embedded Toolchain from the [Arm Developer](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) website. Uncompress the archive and move the `arm-gnu-toolchain-12.2.rel1-darwin-arm64-arm-none-eabi` directory into the `nrf5` directory.

    <!-- is this brew step necessary with the v 12.2.1? You will also need some items from `brew`:

    `brew install gmp mpfr libmpc isl libelf`
     -->

6. Moddable Four uses a modified [Adafruit nRF52 Bootloader](https://github.com/Moddable-OpenSource/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device. Moddable uses the `uf2conv.py` Python tool from Microsoft that packages the UF2 binary for transfer to the device. Download the [uf2conv](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/uf2conv.py) tool. Move or copy the `uf2conv.py` file into the `nrf5` directory.

    Use `chmod` to change the access permissions of `uf2conv` to make it executable.
   
    ```text
    cd ~/nrf5
    chmod 755 uf2conv.py
    ```

6. Download the [Nordic nRF5 SDK](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/nRF5_SDK_17.0.2_d674dde-mod.zip) with Moddable Four modifications.

    Unzip the archive and copy the `nRF5_SDK_17.0.2_d674dde` directory into the `nrf5` directory.

    > See the section [nRF5 SDK modifications](#nrf5-sdk-mods) for information on the modifications to the nRF5 SDK.

7. Setup the `NRF_SDK_DIR` environment variable to point at the nRF5 SDK directory:

    ```text
    export NRF_SDK_DIR=$HOME/nrf5/nRF5_SDK_17.0.2_d674dde
    ```

8. Verify the setup by building `helloworld` for your device target:

    ```text
    cd ${MODDABLE}/examples/helloworld
    mcconfig -d -m -p nrf52/<YOUR_SUBPLATFORM_HERE>
    ```

<a id="mac-troubleshooting"></a>
### Troubleshooting

- If the macOS **DISK NOT EJECTED PROPERLY** remain on your screen, you can download and use the [`ejectfix.py`](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/ejectfix.py) tool to make them auto-dismiss.

   See the [article at the Adafruit blog](https://blog.adafruit.com/2021/05/11/how-to-tone-down-macos-big-surs-circuitpy-eject-notifications/) for details.

<a id="win"></a>
## Windows

<a id="win-instructions"></a>
### Installing

1. Install the Moddable SDK tools by following the instructions in the [Getting Started document](./../Moddable%20SDK%20-%20Getting%20Started.md).

2. Create a `nrf5` directory in your `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>` for required third party SDKs and tools.

    ```text
    cd %USERPROFILE%
    mkdir nrf5
    cd nrf5
    ```

3. Download version 12.2.1 [`AArch32 bare-metal target (arm-none-eabi)`](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-arm-none-eabi.zip) of the GNU Arm Embedded Toolchain from the [Arm Developer](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) website. Uncompress the archive and move the `arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-arm-none-eabi` directory into the `nrf5` directory.

4. Moddable Four uses a modified [Adafruit nRF52 Bootloader](https://github.com/Moddable-OpenSource/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device. `uf2conv.py` is a Python tool from Microsoft that packages the UF2 binary for transfer to the device. Download the [uf2conv](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/uf2conv.py) tool. Move or copy the `uf2conv.py` file into the `nrf5` directory.

5. Download the [Nordic nRF5 SDK](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/nRF5_SDK_17.0.2_d674dde-mod.zip) with Moddable Four modifications.

    Unzip the archive and copy the `nRF5_SDK_17_0_2_d674dde` directory into the `nrf5` directory.

    > See the section [nRF5 SDK modifications](#nrf5-sdk-mods) for information on the modifications to the nRF5 SDK.

6. Setup the `NRF52_SDK_PATH` environment variable to point at your nRF5 SDK directory:

    ```text
    set NRF52_SDK_PATH = %USERPROFILE%\nrf5\nRF5_SDK_17_0_2_d674dde
    ```

7. Download and run the [Python installer](https://www.python.org/ftp/python/2.7.15/python-2.7.15.msi) for Windows. Choose the default options.

8. Edit the system `PATH` environment variable to include the Python directories:

    ```text
    C:\Python27
    C:\Python27\Scripts
    ```

9. Verify the setup by building `helloworld` for your device target:

    ```text
    cd %MODDABLE%\examples\piu\balls
    mcconfig -d -m -p nrf52/moddable_four
    ```

<a id="win-troubleshooting"></a>
### Troubleshooting

<a id="lin"></a>
## Linux

<a id="lin-instructions"></a>
### Installing

1. Install the Moddable SDK tools by following the instructions in the [Getting Started document](./../Moddable%20SDK%20-%20Getting%20Started.md).

2. Create a `nrf5` directory in your home directory at `~/nrf5` for required third party SDKs and tools.

    ```text
    cd $HOME
    mkdir nrf5
    cd nrf5
    ```

3. Download version 12.2.1 [AArch32 bare-metal target (arm-none-eabi)](https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz) of the GNU Arm Embedded Toolchain from the [Arm Developer](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) website. Uncompress the archive and move the `arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi` directory into the `nrf5` directory.

4. Moddable Four uses a modified [Adafruit nRF52 Bootloader](https://github.com/Moddable-OpenSource/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device. `uf2conv.py` is a Python tool from Microsoft that packages the UF2 binary for transfer to the device. Download the [uf2conv](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/uf2conv.py) tool. Move or copy the `uf2conv.py` file into the `nrf5` directory.

5. Download the [Nordic nRF5 SDK](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/nRF5_SDK_17.0.2_d674dde-mod.zip) with Moddable Four modifications.

    Unzip the archive and copy the `nRF5_SDK_17_0_2_d674dde` directory into the `nrf5` directory.

    > See the section [nRF5 SDK modifications](#nrf5-sdk-mods) for information on the modifications to the nRF5 SDK.

6. Setup the `NRF_SDK_DIR` environment variable to point at the nRF5 SDK directory:

    ```text
    export NRF_SDK_DIR=$HOME/nrf5/nRF5_SDK_17.0.2_d674dde
    ```

7. Verify the setup by building `helloworld` for your device target:

    ```text
    cd ${MODDABLE}/examples/helloworld
    mcconfig -d -m -p nrf52/<YOUR_SUBPLATFORM_HERE>
    ```

<!--
<a id="lin-troubleshooting"></a>
### Troubleshooting
-->

<a id="troubleshooting"></a>
# Troubleshooting

### `region RAM overflowed with stack`

If you are building an application and the link fails with an error `arm-none-eabi/bin/ld: region RAM overflowed with stack`, you will need to reduce the amount of RAM allocated to the heap.

By default, `HEAP_SIZE` is set to 0x32800.

In your application manifest, you can change the `HEAP_SIZE` allocation:

```text
    "build": {
        "HEAP_SIZE": "0x34000"
     }
```

<a id="advanced"></a>
# Advanced

<a id="debugging-native-code"></a>
## Debugging Native Code

As with all Moddable platforms, you can debug script code using `xsbug` over the USB serial interface with Moddable Four. For more information, see the [`xsbug` documentation](../../xs/xsbug.md). For native code source level debugging, developers can use [GDB](https://www.gnu.org/software/gdb/documentation/).

Debugging native code on the Moddable Four requires a [Nordic nRF52840-DK board](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK), [Segger J-Link Plus](https://www.segger.com/products/debug-probes/j-link/models/j-link-plus/) or compatible device.

<a id="j-link-connection"></a>
For example, connect your Moddable Four to the nRF52840-DK board as follows:

| nRF52840 DK | Moddable Four | 
| :---: | :---: | :---
| SWD CLK | SWDCLK |
| SWD IO | SWDIO |
| RESET | RESET | (optional)
| GND DETECT | GND |
| VTG | 3V3 |

<img src="../assets/devices/moddable-four-dk-pinout.png" width="100%"><BR>

If you have a Segger J-Link Plus, the connections to the J-Link are as follows:

| Moddable Four | J-Link |
| :---: | :---: | :---
| SWDCLK | TCK |
| SWDIO | TMS |
| RESET | RESET | (optional)
| GND | GND |
| 3V3 | VTref |



<img src="../assets/devices/nrf52-jlink.png" width="50%"><BR>



[GDB](https://www.gnu.org/software/gdb/documentation/) is the GNU debugger widely used on Unix-like build hosts to debug native code. GDB is included in the Arm Embedded Toolchain archive downloaded during the [SDK and Host Environment Setup](#setup) step.

GDB communicates with the nRF58240 device via a J-Link connection in the nRF52840-DK board, Segger j-link Plus or other J-Link compatible device. Take the following steps to install/configure the required tools and launch GDB:

1. [Install the nRF Command Line Tools](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools). Make sure the `JLinkGDBServer` is somewhere in your `$PATH`.

2. Create a GDB startup command text file `.gdbinit` in your home directory with the following contents:

    ```text
    target remote localhost:2331
    mon speed 10000
    mon flash download=1
    load
    break main
    mon reset 0
    continue
    ```

3. Connect both your device and debugger USB ports to your computer. Both USB ports can be connected to the computer via a USB hub.

4. Build the Moddable app that includes the native code you plan to debug. For this example, we build the BLE [heart-rate-server](https://github.com/Moddable-OpenSource/moddable/tree/public/examples/network/ble/heart-rate-server) example:

    ```text
    cd $MODDABLE/examples/network/ble/heart-rate-server
    mcconfig -d -m -p nrf52/moddable_four -t build
    ```

5. Launch the J-Link GDB server from a command line console:

    ```text
    JLinkGDBServer -device nRF52840_xxAA -if swd -port 2331
    ```

    The GDB server will connect to the nRF52840-DK target and wait for a client connection:

    ```text
    Connecting to J-Link...
    J-Link is connected.
    Firmware: J-Link OB-SAM3U128-V2-NordicSemi compiled Mar 17 2020 14:43:00
    Hardware: V1.00
    S/N: 683214408
    Checking target voltage...
    Target voltage: 3.30 V
    Listening on TCP/IP port 2331
    Connecting to target...Connected to target
    Waiting for GDB connection...
    ```

6. From a second command line console, launch the GDB client, passing the application ELF and GDB startup command text files as command line arguments:

    ```text
    arm-none-eabi-gdb $MODDABLE/build/tmp/nrf52/moddable_four/debug/heart-rate-server/xs_nrf52.out -x ~/nrf5/gdb_cmds.txt
    ```
    
    The GDB server connects with the client, downloads the application and stops at the breakpoint `main` specified in the GDB setup command file:

    ```text
    Breakpoint 1 at 0x46550: file /Users/<user>/Projects/moddable/build/devices/nrf52/xsProj/main.c, line 149.
Resets core & peripherals via SYSRESETREQ & VECTRESET bit.
    
    Breakpoint 1, main () at /Users/<user>/Projects/moddable/build/devices/nrf52/xsProj/main.c:149
    
    149     clock_init();
    (gdb)
    ```

7. At the `(gdb)` prompt, type `c` to continue execution and/or set other breakpoints, etc...

<a id="bootloader"></a>
## Bootloader

Applications using the Moddable SDK running on the nrf52 SoC typically use a modified [Adafruit nRF52 Bootloader](https://github.com/Moddable-OpenSource/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device.

### Programming mode

Moddable tools will put the device into programming mode automatically if the device has been programmed with a debug version of an app.

Otherwise **double-tap** the reset button on the device to put it into programming mode. The on-board LED will blink every second and a USB disk named **MODDABLE4** will appear on your desktop.

Drag a `.uf2` file to the device to program it.

> Note: The bootloader can be updated in a similar fashion.

> Note: When the device has a debug version of a Moddable app loaded on it, the Moddable tools can set the device to auto-program so you do not have to reset the board.



<a id="install-bootloader"></a>
### Installing the bootloader

To use a nRF52840 device with the Moddable SDK, you will have to install the bootloader to that device. This will replace the functionality of the previous bootloader.

> Note: The Moddable Four has the bootloader pre-installed.

> Note: You may brick your device.

You will need a Segger J-Link or equivalent to program the bootloader.

1. Connect your device to the J-Link in the same way that you would for the debugger. See the 
[Debugging Native Code](#debugging-native-code)
section.

2. Fetch the bootloader repository:
   
   ```
   git clone https://github.com/Moddable-OpenSource/Adafruit_nRF52_Bootloader --recurse-submodules
   ```

3. Build for your device
   
   ```
   cd Adafuit_nRF52_Bootloader
   make BOARD=moddable_four
   ```
   
   > Note: The following BOARD configurations have been updated to support Moddable.
   - `moddable_four`
   - `moddable_pca10056`
   - `moddable_sparkfun52840`
   - `moddable_makerdiary_nrf52`
 

4. Install to your device. First install the SoftDevice, then flash the bootloader:
   
   ```
   make BOARD=moddable_four sd
   make BOARD=moddable_four flash
   ```

5. Double-tap the reset button to set the device to Programming mode. The LED will blink regularly, and the `MODDABLEnRF` volume will appear on the desktop.

You can now program the device.


----
        
<a id="nrf5-sdk-mods"></a>
## nRF5 SDK modifications 

Moddable Four requires a few small adjustments to the Nordic nRF5 SDK. You can use the prepared SDK at [Nordic nRF5 SDK](https://github.com/Moddable-OpenSource/tools/releases/download/v1.0.0/nRF5_SDK_17.0.2_d674dde-mod.zip).

Or you can make your own by following these steps to modify the SDK:

1. Download the [Nordic nRF5 SDK](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download) by taking the following steps: 

    - Select `v17.0.2` from the nRF5 SDK versions section.
    
        ![](../assets/devices/nrf5-sdk-versions.png) 

    - Uncheck all SoftDevices.

        ![](../assets/devices/softdevices.png)

    - Click the **Download Files** button at the bottom of the page. You should see the same selection as in the image below.

        ![](../assets/devices/nrf5-sdk-selected.png)

    The downloaded archive is named `DeviceDownload.zip`. Unzip the archive and copy the `nRF5_SDK_17.0.2_d674dde` directory into the `nrf5` directory.

2. Setup the `NRF_SDK_DIR` environment variable to point at the nRF5 SDK directory:

    ```text
    export NRF_SDK_DIR=$HOME/nrf5/nRF5_SDK_17.0.2_d674dde
    ```

3. Add a board definition file for the Moddable Four to the Nordic nRF5 SDK. The board definition file includes Moddable Four LED, button and pin definitions. To add the Moddable Four board definition file, take the following steps:

    - The `moddable_four.h` board definition file is found in `$MODDABLE/build/devices/nrf52/config/moddable_four.h`. Copy the `moddable_four.h` file to the Nordic nRF5 SDK `components/boards/` directory.

    ```text
    cp $MODDABLE/build/devices/nrf52/config/moddable_four.h $NRF_SDK_DIR/components/boards
    ```

    - Modify `$NRF_SDK_DIR/components/boards/boards.h`, adding the following before `#elif defined(BOARD_CUSTOM)`:

    ```c
    #elif defined (BOARD_MODDABLE_FOUR)
      #include "moddable_four.h"
    ```

4. Add `SPIM3` support:

    The nRF5 SDK has a file `integration/nrfx/legacy/apply_old_config.h` that needs a small change. Add `|| NRFX_SPIM3_ENABLED` after the `NRFX_SPIM2_ENABLED` as shown in the line below:

    ```text
    #define NRFX_SPIM_ENABLED \
    (SPI_ENABLED && (NRFX_SPIM0_ENABLED || NRFX_SPIM1_ENABLED || NRFX_SPIM2_ENABLED || NRFX_SPIM3_ENABLED))
    ```

5. Enable LE secure connection support:

	Disable the stack overflow check in the `nrf_stack_info_overflowed` function In the Nordic SDK `nrf_stack_info.h` file:

    ```c
    __STATIC_INLINE bool nrf_stack_info_overflowed(void)
    {
    #if 0
        if (NRF_STACK_INFO_GET_SP() < NRF_STACK_INFO_BASE)
        {
            return true;
        }
    #endif
        return false;
    }
    ```
