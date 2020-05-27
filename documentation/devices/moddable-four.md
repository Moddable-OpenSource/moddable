# Getting Started with Moddable Four

Copyright 2020 Moddable Tech, Inc.<BR>
Revised: May 27, 2020

This document describes how to start building Moddable applications for Moddable Four. It provides information on how to configure host build environments, how to build and deploy apps, and includes links to external development resources.

## Table of Contents

- [About Moddable Four](#about-moddable-four)
	- [Components](#components)
	- [Pinout](#pinout)
- [SDK and Host Environment Setup](#setup)
	- [macOS](#macos-setup)
	- [Windows](#windows-setup)
	- [Linux](#linux-setup)
-  [Enabling LE secure connection support](#le-secure-connections)
- [Development Resources](#development-resources)
	- [Examples](#examples)
	- [Documentation](#documentation)
	- [Support](#support)
	- [Updates](#updates)
- [Advanced](#advanced)
	- [Debugging Native code](#debugging-native-code)
	- [Debugging Native and Script Code](#debugging-native-and-script-code)
	- [Bootloader](#bootloader)

<a id="about-moddable-four"></a>
## About Moddable Four

<!--TO DO: add image-->
<img src="../assets/devices/moddable-four.png">

Moddable Four is a low-power, Bluetooth LE development board that makes it easy for developers to experiment with the Moddable SDK. It is available to purchase on the [Moddable website](http://www.moddable.com/purchase).

<a id="components"></a>
### Components

The two main components of Moddable Four are the nRF52840 module and mirror display. The nRF52840 module includes a BLE antenna, 1 MB Flash, and 256 KB RAM. The Sharp mirror display is a 128x128 black and white display that uses the [`ls013b4dn04` display driver](../drivers/ls013b4dn04/ls013b4dn04.md).

It also includes an integrated LIS3DH accelerometer, jog dial, and CR2032 battery connector.

<a id="pinout"></a>
### Pinout

<img src="../assets/devices/moddable-four-pinout.png">

**Note:** LCD-PWR is not for arbitrary digital inputs/outputs. It is used to provide power to a sensor and to the screen. 

- Writing `0` to GPIO23 emits 3.3V on LCD-PWR, which also gives power to the screen. 
- Writing `1` to GPIO23 turns off the the pin and the screen.

<a id="setup"></a>
## SDK and Host Environment Setup

<a id="macos-setup"></a>

### macOS setup

1. The [Moddable SDK Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md) describes how to configure the host build environment and install the required SDKs, drivers, and development tools. Follow the instructions in the [Host environment setup](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md#host-mac) section for macOS.

2. Create a `nrf5` directory in your home directory at `~/nrf5` for required third party SDKs and tools.

	```text
	cd $HOME
	mkdir nrf5
	cd nrf5
	```

3. Download version [`8-2018-q4-major`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-mac.tar.bz2?revision=b88d4399-9897-465a-9363-ace86610e48c?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Mac%20OS%20X,8-2018-q4-major) of the GNU Arm Embedded Toolchain from the [GNU-RM Downloads](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) website. Untar the archive and copy the `gcc-arm-none-eabi-8-2018-q4-major` directory into the `nrf5` directory.

	> **Note:** Other versions of the GNU tools may work, but `8-2018-q4-major` is the version we currently support.

4. Moddable Four uses a modified [Adafruit nRF52 Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device. `uf2conv.py` is a Python tool from Microsoft that packages the UF2 binary for transfer to the device. Download the [uf2conv](http://test.moddable.com/private/nrf52/uf2conv.zip) tool. Unzip the archive and copy the `uf2conv.py` file into the `nrf5` directory.

	Use `chmod` to change the access permissions of `uf2conv` to make it executable.
	
	```text
	cd ~/nrf5
	chmod 755 uf2conv.py 
	```

5. Download the [Nordic nRF5 SDK](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download) by taking the following steps:

	- Select `v15.3.0` from the nRF5 SDK versions section.

		![](../assets/devices/nrf5-sdk-versions.png)

	- Uncheck all SoftDevices.

		![](../assets/devices/softdevices.png)

	- Click the **Download Files** button at the bottom of the page. You should see the same selection as in the image below.

		![](../assets/devices/selected.png)

	The downloaded archive is named `DeviceDownload.zip`. Unzip the archive and copy the `nRF5_SDK_15.3.0_59ac345` directory into the `nrf5` directory.

6. Setup the `NRF_SDK_DIR` environment variable to point at the nRF5 SDK directory:
	
	```text
	export NRF_SDK_DIR=$HOME/nrf5/nRF5_SDK_15.3.0_59ac345
	```

7. Add a board definition file for the Moddable Four to the Nordic nRF5 SDK. The board definition file includes Moddable Four LED, button and pin definitions. To add the Moddable Four board definition file, take the following steps:

	- The `moddable_four.h` board definition file is found in `$MODDABLE/build/devices/nrf52/config/moddable_four.h`. Copy the `moddable_four.h` file to the Nordic nRF5 SDK `components/boards/` directory.

	```text
	cp $MODDABLE/build/devices/nrf52/config/moddable_four.h $NRF_SDK_DIR/components/boards
	```

	- Modify `$NRF_SDK_DIR/components/boards/boards.h`, adding the following before `#elif defined(BOARD_CUSTOM)`:

	```c
	#elif defined (BOARD_MODDABLE_FOUR)
	  #include "moddable_four.h"
	```

<a id="macOS-building-and-deploying-apps"></a>
#### Building and Deploying Apps

After you've setup your macOS host environment, take the following steps to install an application on your Moddable Four.

1. Attach your Moddable Four to your computer with a micro USB cable.

	Make sure you're using a data-sync capable cable, not one that is power-only.

2. Put the device into programming mode by double-tapping the RESET button or by holding the BOOT button while tapping RESET.

	Programming mode is indicated by the LED indicator staying lit at boot time. A disk named `MODDABLE4` will also appear on your desktop.

	![Moddable Four disk](../assets/devices/moddable-four-M4-disk.png)

	> **Note:** If you do not program your device within a short period, it will reboot to the installed application.

3. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Specify the platform `-p nrf52/moddable_four` with `mcconfig` to build for Moddable Four. Build the [`piu/balls`](../../examples/piu/balls) example:
	
	```text
	cd $MODDABLE/examples/piu/balls
	mcconfig -d -m -p nrf52/moddable_four
	```
	
	The [examples readme](../../examples) contains additional information about other commonly used `mcconfig` arguments for screen rotation and more.
	
<a id="windows-setup"></a>

### Windows setup

1. The [Moddable SDK Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md) describes how to configure the host build environment and install the required SDKs, drivers, and development tools. Follow the instructions in the [Host environment setup](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md#host-windows) section for Windows.

2. Create a `nrf5` directory in your `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>` for required third party SDKs and tools.

	```text
	cd %USERPROFILE%
	mkdir nrf5
	cd nrf5
	```

3. Download version [`7-2017-q4-major`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2017q4/gcc-arm-none-eabi-7-2017-q4-major-win32.zip?revision=df1b65d3-7c8d-4e82-b114-e0b6ad7c2e6d?product=GNU%20Arm%20Embedded%20Toolchain,ZIP,,Windows,7-2017-q4-major) of the 32-bit GNU Arm Embedded Toolchain from the [GNU-RM Downloads](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) website. Unzip the archive and copy the `gcc-arm-none-eabi-7-2017-q4-major-win32` directory into the `nrf5` directory.

	> **Note:** Newer versions of the GNU Arm Embedded Toolchain for Windows are not supported due to [issues](https://bugs.launchpad.net/gcc-arm-embedded/+bug/1810274) with `objcopy.exe`.

4. Moddable Four uses a modified [Adafruit nRF52 Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device. `uf2conv.py` is a Python tool from Microsoft that packages the UF2 binary for transfer to the device. Download the [uf2conv](http://test.moddable.com/private/nrf52/uf2conv.zip) tool. Unzip the archive and copy the `uf2conv.py` file from the extracted `uf2conv` directory into the `nrf5` directory.

5. Download the [Nordic nRF5 SDK](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download) by taking the following steps:

	- Select `v15.3.0` from the nRF5 SDK versions section.

		![](../assets/devices/nrf5-sdk-versions.png)

	- Uncheck all SoftDevices.

		![](../assets/devices/softdevices.png)

	- Click the **Download Files** button at the bottom of the page. You should see the same selection as in the image below.

		![](../assets/devices/selected.png)

	The downloaded archive is named `DeviceDownload.zip`. Unzip the archive and copy the `nRF5_SDK_15.3.0_59ac345` directory into the `nrf5` directory.

6. Setup the `NRF52_SDK_PATH` environment variable to point at your nRF5 SDK directory:

	```text
	set NRF52_SDK_PATH = %USERPROFILE%\nrf5\nRF5_SDK_15.3.0_59ac345
	```

7. Download and run the [Python installer](https://www.python.org/ftp/python/2.7.15/python-2.7.15.msi) for Windows. Choose the default options.

8. Edit the system `PATH` environment variable to include the Python directories:

	```text
	C:\Python27
	C:\Python27\Scripts
	```

9. Add a board definition file for the Moddable Four to the Nordic nRF5 SDK. The board definition file includes Moddable Four LED, button and pin definitions. To add the Moddable Four board definition file, take the following steps:

	- The `moddable_four.h` board definition file is found in `%MODDABLE%\build\devices\nrf52\config\moddable_four.h`. Copy the `moddable_four.h` file to the Nordic nRF5 SDK `components\boards\` directory.

	```text
	copy %MODDABLE%\build\devices\nrf52\config\moddable_four.h %NRF52_SDK_PATH%\components\boards
	```

	- Modify the `%NRF52_SDK_PATH%\components\boards.h` file, adding the following before `#elif defined(BOARD_CUSTOM)`:

	```c
	#elif defined (BOARD_MODDABLE_FOUR)
	  #include "moddable_four.h"
	```

<a id="windows-building-and-deploying-apps"></a>
#### Building and Deploying Apps

After you've setup your Windows host environment, take the following steps to install an application on your Moddable Four.

1. Attach your Moddable Four to your computer with a micro USB cable.

	Make sure you're using a data-sync capable cable, not one that is power-only.

2. Put the device into programming mode by double-tapping the RESET button or by holding the BOOT button while tapping RESET.

	Programming mode is indicated by the LED indicator staying lit at boot time. A disk named `MODDABLE4` will also appear in File Explorer.

	![Moddable Four disk](../assets/devices/moddable-four-M4-disk-win.png)

	> **Note:** If you do not program your device within a short period, it will reboot to the installed application.

3. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Specify the platform `-p nrf52/moddable_four` with `mcconfig` to build for Moddable Four. Build the [`piu/balls`](../../examples/piu/balls) example:
	
	```text
	cd %MODDABLE%\examples\piu\balls
	mcconfig -d -m -p nrf52/moddable_four
	```
	
	The [examples readme](../../examples) contains additional information about other commonly used `mcconfig` arguments for screen rotation and more.
	
<a id="linux-setup"></a>

### Linux setup

1. The [Moddable SDK Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md) describes how to configure the host build environment and install the required SDKs, drivers, and development tools. Follow the instructions in the [Host environment setup](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/Moddable%20SDK%20-%20Getting%20Started.md#host-linux) section for Linux.

2. Create a `nrf5` directory in your home directory at `~/nrf5` for required third party SDKs and tools.

	```text
	cd $HOME
	mkdir nrf5
	cd nrf5
	```

3. Download version [`8-2018-q4-major`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2?revision=ab7c81a3-cba3-43be-af9d-e922098961dd?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,8-2018-q4-major) of the 64-bit GNU Arm Embedded Toolchain from the [GNU-RM Downloads](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) website. Untar the archive and copy the `gcc-arm-none-eabi-8-2018-q4-major` directory into the `nrf5` directory.

	> **Note:** Other versions of the GNU tools may work, but `8-2018-q4-major` is the version we currently support.

4. Moddable Four uses a modified [Adafruit nRF52 Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader) that supports the UF2 file format for flashing firmware to a device. `uf2conv.py` is a Python tool from Microsoft that packages the UF2 binary for transfer to the device. Download the [uf2conv](http://test.moddable.com/private/nrf52/uf2conv.zip) tool. Unzip the archive and copy the `uf2conv.py` file into the `nrf5` directory.

	Use `chmod` to change the access permissions of `uf2conv` to make it executable.
	
	```text
	cd ~/nrf5
	chmod 755 uf2conv.py 
	```

5. Download the [Nordic nRF5 SDK](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download) by taking the following steps:

	- Select `v15.3.0` from the nRF5 SDK versions section.

		![](../assets/devices/nrf5-sdk-versions.png)

	- Uncheck all SoftDevices.

		![](../assets/devices/softdevices.png)

	- Click the **Download Files** button at the bottom of the page. You should see the same selection as in the image below.

		![](../assets/devices/selected.png)

	The downloaded archive is named `DeviceDownload.zip`. Unzip the archive and copy the `nRF5_SDK_15.3.0_59ac345` directory into the `nrf5` directory.

6. Setup the `NRF_SDK_DIR` environment variable to point at the nRF5 SDK directory:
	
	```text
	export NRF_SDK_DIR=$HOME/nrf5/nRF5_SDK_15.3.0_59ac345
	```

7. Add a board definition file for the Moddable Four to the Nordic nRF5 SDK. The board definition file includes Moddable Four LED, button and pin definitions. To add the Moddable Four board definition file, take the following steps:

	- The `moddable_four.h` board definition file is found in `$MODDABLE/build/devices/nrf52/config/moddable_four.h`. Copy the `moddable_four.h` file to the Nordic nRF5 SDK `components/boards/` directory.

	```text
	cp $MODDABLE/build/devices/nrf52/config/moddable_four.h $NRF_SDK_DIR/components/boards
	```

	- Modify `$NRF_SDK_DIR/components/boards/boards.h`, adding the following before `#elif defined(BOARD_CUSTOM)`:

	```c
	#elif defined (BOARD_MODDABLE_FOUR)
	  #include "moddable_four.h"
	```

<a id="linux-building-and-deploying-apps"></a>
#### Building and Deploying Apps

After you've setup your Linux host environment, take the following steps to install an application on your Moddable Four.

1. Attach your Moddable Four to your computer with a micro USB cable.

	Make sure you're using a data-sync capable cable, not one that is power-only.

2. Put the device into programming mode by double-tapping the RESET button or by holding the BOOT button while tapping RESET.

	Programming mode is indicated by the LED indicator staying lit at boot time. A disk named `MODDABLE4` will also appear on your desktop.

	![Moddable Four disk](../assets/devices/moddable-four-M4-disk-linux.png)

	> **Note:** If you do not program your device within a short period, it will reboot to the installed application.

3. Build and deploy the app with `mcconfig`.

	`mcconfig` is the command line tool to build and launch Moddable apps on microcontrollers and the simulator. Full documentation of `mcconfig` is available [here](../tools/tools.md). 
	
	Specify the platform `-p nrf52/moddable_four` with `mcconfig` to build for Moddable Four. Build the [`piu/balls`](../../examples/piu/balls) example:
	
	```text
	cd $MODDABLE/examples/piu/balls
	mcconfig -d -m -p nrf52/moddable_four
	```
	
	The [examples readme](../../examples) contains additional information about other commonly used `mcconfig` arguments for screen rotation and more.
	
<a id="le-secure-connections"></a>
## Enabling LE secure connection support

LE secure connection support is disabled by default in the Moddable build due to a FreeRTOS incompatibility in the Nordic SDK. To enable LE secure connections, the Nordic SDK and config must be patched as follows:

1. Edit the two `#define` statements in the Moddable SDK [`sdk_config.h`](../build/devices/nrf52/config/sdk_config.h) file to enable LE secure connection support:

	```c
	#ifndef NRF_BLE_LESC_ENABLED
		#define NRF_BLE_LESC_ENABLED 1
	#endif
	
	#ifndef PM_LESC_ENABLED
		#define PM_LESC_ENABLED 1
	#endif
	``` 
	
2. Disable the stack overflow check in the `nrf_stack_info_overflowed` function In the Nordic SDK `nrf_stack_info.h` file:

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

> **Note:** Because of this incompatibility, the Moddable BLE server traces a warning to the `xsbug` console when LE secure connections are enabled.
	
<a id="development-resources"></a>
## Development Resources

<a id="examples"></a>
### Examples

The Moddable SDK has over 150 [example apps](../../examples) that demonstrate how to use its many features. Many of these examples run on Moddable Four. 

That said, many of the examples that use Commodetto and Piu are designed for colored QVGA screens. In addition, not every example is compatible with Moddable Four hardware. Some examples are designed to test specific display and touch drivers that are not compatible with the Moddable Four display and give a build error.

<a id="documentation"></a>
### Documentation

Documentation for the nRF5 device and SDK can be found on the [Nordic Semiconductor Infocenter](https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html). Of particular interest is the documentation for the Nordic nRF5 SDK v15.3.0, which is available [here](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/index.html?cp=7_5_0).

All the documentation for the Moddable SDK is in the [documentation](../) directory. The **documentation**, **examples**, and **modules** directories share a common structure to make it straightforward to locate information. Some of the highlights include: 

- The `commodetto` subdirectory, which contains resources related to Commodetto--a bitmap graphics library that provides a 2D graphics API--and Poco, a lightweight rendering engine.
- The `piu` subdirectory, which contains resources related to Piu, a user interface framework that makes it easier to create complex, responsive layouts.
- The `networking` subdirectory, which contains networking resources related to network sockets and a variety of standard, secure networking protocols built on sockets including HTTP/HTTPS, WebSockets, DNS, SNTP, and telnet
- The `pins` subdirectory, which contains resources related to supported hardware protocols (digital, analog, PWM, I2C, etc.). A number of drivers for common off-the-shelf sensors and corresponding example apps are also available.

<a id="support"></a>
### Support

If you have questions, we recommend you [open an issue](https://github.com/Moddable-OpenSource/moddable/issues). We'll respond as quickly as practical, and other developers can offer help and benefit from the answers to your questions. Many questions have already been answered, so please try searching previous issues before opening a new issue.

<a id="updates"></a>
### Updates

The best way to keep up with what we're doing is to follow us on Twitter ([@moddabletech](https://twitter.com/moddabletech)). We post announcements about new posts on [our blog](http://blog.moddable.com/) there, along with other Moddable news.

<a id="advanced"></a>
## Advanced

This section provides information about debugging and the bootloader on Moddable Four.

<a id="debugging-native-code"></a>
### Debugging Native code

As with all Moddable platforms, you can debug script code using `xsbug` over the USB serial interface with Moddable Four. For more information, see the [`xsbug` documentation](../../xs/xsbug.md).

Some developers may need to debug native code. [SEGGER Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/) is a C/C++ IDE for embedded systems. It includes a debugger that allows you to set breakpoints and examine registers, variables, and memory. You can debug native code on Moddable Four using the SEGGER Embedded Studio debugger. For documentation on using the debugger, see the [documentation by SEGGER](https://www.segger.com/products/development-tools/embedded-studio/technology/debugger/).

Debugging native code on the Moddable Four with SEGGER Embedded Studio requires a [Nordic nRF52840 DK board](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK). Connect your Moddable Four to the nRF52840 DK board as follows:

| nRF52840 DK | Moddable Four |
| :---: | :---: |
| SWD CLK | SDWCLK |
| SWD IO | SWDIO |
| RESET | RESET |
| GND DETECT | GND |
| VTG | 3V3 |

![Moddable Four to nRF52840 DK connection](../assets/devices/moddable-four-dk-pinout.png)

<!--In SEGGER Embedded Studio, open the Options dialog for the `xsproj` project. Go to **Debug->J-Link** and set the **Target Interface Type** to **SWD**, as shown in the image below.

![J-Link->Target Interface Type in SEGGER Embedded Studio](../assets/devices/target-interface-type.png)

<a id="debugging-native-and-script-code"></a>
### Debugging Native and Script Code

`xsbug` and the SEGGER Embedded Studio debugger can be used simultaneously. To do so, your Moddable Four must be connected to the nRF52840 DK using an external FTDI serial interface as follows:

| nRF52840 DK | FTDI interface |
| :---: | :---: |
| ? | ? |
| ? | ? |
| ? | ? |

Then you'll need to launch launch `xsbug` and `serial2xsbug` by taking the following steps:

- Identify the serial port that the interface is using. Start with your device unplugged. Enter the following command in a terminal window.

	```text
	ls dev/cu.*
	```
	
	Then plug your device in and enter the same command. There should be an additional device file this time.
	
	The image below shows the terminal output when the command is entered before and after plugging a Moddable Four in. In this example, the Moddable Four is `/dev/cu.usbserial-AL035YB2`.

	![Identify FTDI port](./nrf52/assets/identifyFTDI.png)

- Enter the following commands to launch `xsbug` and `serial2xsbug`. Replace `<DEVICE_FILE>` with the serial port that you identified.

	```text
	xsbug &
	serial2xsbug <DEVICE_FILE> 115200 8N1
	```
	
	For example:
	
	```text
	xsbug &
	serial2xsbug /dev/cu.usbserial-AL035YB2115200 8N1
	```
	
	-->

<a id="bootloader"></a>
### Bootloader

Moddable Four uses a slightly modified [Adafruit nRF52 Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader). It is very unlikely you will need to build a bootloader; the bootloader is pre-installed on Moddable Four and a pre-built version of the bootloader is included on the device and in the source tree.

In the unlikely event that you need to build a bootloader, take the following steps. Be careful when you do so, as you can brick your device.

1. Get the bootloader sources, enter the repository directory and update the submodules.

	```	text
	git clone https://github.com/adafruit/Adafruit_nRF52_Bootloader.git
	cd Adafruit_nRF52_Bootloader
	git submodule update --init --recursive
	```

2. From the repository directory, add the Moddable Four configuration.

	```text
	cp -r $MODDABLE/build/devices/nrf52/config/bootloader/moddable_four src/boards
	```

3. Edit the file `src/usb/uf2/uf2cfg.h` to expand available flash.

	Change:

	```text
    #define USER_FLASH_END     0xAD000 // Fat Fs start here
	```
	to:

	```text
	#define USER_FLASH_END     0xF4000 // use all of flash up to the bootloader
	```

4. Build the bootloader.

	```text
	make BOARD=moddable_four all
	```

5. With the board hooked up to a DK through SWD interface, flash the softdevice to the Moddable Four:

	```text
	make BOARD=moddable_four sd
	```

6. With the board hooked up to a DK through SWD interface, flash the bootloader to the Moddable Four:

	```text
	make BOARD=moddable_four flash
	```

7. Remove the board from the programmer. It is now ready for use.

