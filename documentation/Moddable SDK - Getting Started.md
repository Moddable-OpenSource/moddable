# Moddable SDK – Getting Started
Copyright 2016-2019 Moddable Tech, Inc.<BR>
Revised: November 11, 2019

This document provides an introduction to getting started building apps with the Moddable SDK. It describes how to configure the host build environments, install the required SDKs, drivers and development tools, build applications, and use `xsbug`, the JavaScript source code debugger.

## Table of Contents

* [Overview](#overview)
	* [Platforms](#platforms)
* [macOS](#mac)
	* [Host environment setup](#host-mac)
	* [ESP8266](#esp8266-mac)
	* [ESP32](#esp32-mac)
* [Windows](#windows)
	* [Host environment setup](#host-windows)
	* [ESP8266](#esp8266-windows)
	* [ESP32](#esp32-windows)
* [Linux](#linux)
	* [Host environment setup](#host-linux)
	* [ESP8266](#esp8266-linux)
	* [ESP32](#esp32-linux)
* [Debugging applications](#debugging-applications)
* [ESP8266 Arduino version 2.4](#arduino-version)

<a id="overview"></a>
## Overview

Before you can build applications, you need to set up your host environment and install the required drivers and development tools for your target platform.

The instructions below will have you verify your setup by running the `helloworld` example on your target platform using `mcconfig`, a command line tool that builds and runs Moddable applications. You use a platform identifier to specify which target `mcconfig` should build for. This document contains instructions for the ESP8266 and ESP32 platforms, which use the platform identifiers `esp` and `esp32`, respectively. For example:

```
mcconfig -d -m -p esp
```

To build for other ESP8266 or ESP32-based devices--for example the Moddable One and Moddable Two--you must also specify a subplatform. For example:

```
mcconfig -d -m -p esp/moddable_one
```

A list of available platforms/subplatforms is provided in the **Platforms** section below.

More detailed getting started guides are available for the following devices:

- [Moddable Zero](./devices/moddable-zero.md)
- [Moddable One](./devices/moddable-one.md)
- [Moddable Two](./devices/moddable-two.md)
- [Moddable Three](./devices/moddable-three.md)
- [Silicon Labs Gecko devices](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/gecko/GeckoBuild.md)
- [Qualcomm QCA4020](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/qca4020/README.md)

<a id="platforms"></a>
### Platforms

| Identifier | Description |
| :--- | :--- |
| mac | macOS target
| win | Windows target
| lin | Linux target
| esp | ESP8266 device target
| esp/moddable_zero | [Moddable Zero](./devices/moddable-one.md) device target
| esp/moddable_one | [Moddable One](./devices/moddable-one.md) device target
| esp32/moddable_two | [Moddable Two](./devices/moddable-two.md) device target
| esp/moddable_three | [Moddable Three](./devices/moddable-three.md) device target
| esp/adafruit_oled | [Adafruit OLED display + ESP8266](../documentation/displays/wiring-guide-adafruit-OLED.md)
| esp/adafruit_st7735 | [Adafruit 1.8" ST7735 display + ESP8266](../documentation/displays/wiring-guide-adafruit-1.8-st7735.md)
| esp/crystalfontz\_monochrome\_epaper | [Crystalfontz monochrome ePaper display + ESP8266](../documentation/displays/wiring-guide-crystalfontz-eink.md)
| esp/sharp_memory | [2.7" Sharp Memory display + ESP8266](../documentation/displays/wiring-guide-sharp-memory-2.7-spi.md)
| esp/sharp\_memory\_square | [1.3" Sharp Memory Display + ESP8266](../documentation/displays/wiring-guide-sharp-memory-1.3-spi.md)
| esp/sparkfun_teensyview | [SparkFun TeensyView + ESP8266](../documentation/displays/wiring-guide-sparkFun-teensyview-spi.md)
| esp/switch\_science\_reflective\_lcd | [Switch Science reflective LCD display + ESP8266](../documentation/displays/wiring-guide-switch-science-LCD.md)
| esp/buydisplay_ctp | [BuyDisplay 2.8" CTP display + ESP8266](../documentation/displays/wiring-guide-generic-2.8-CPT-spi.md)
| esp32 | ESP32 device target
| esp32/moddable_zero | [ESP32 with generic SPI display](../documentation/displays/wiring-guide-generic-2.4-spi-esp32.md)
| esp32/lilygo_t5s | [LilyGO TTGO T5S](https://github.com/LilyGO/TTGO-T5S-Epaper)
| esp32/lilygo_taudio | [LilyGO TTGO TAudio](https://github.com/LilyGO/TTGO-TAudio)
| esp32/m5stack | [M5Stack](https://www.aliexpress.com/store/product/M5Stack-Official-Stock-Offer-ESP32-Basic-Core-Development-Kit-Extensible-Micro-Control-Wifi-BLE-IoT-Prototype/3226069_32837164440.html?spm=2114.12010615.8148356.2.11c127aeBNzJBb)
| esp32/m5stack_fire | [M5Stack Fire](https://www.aliexpress.com/store/product/M5Stack-NEW-PSRAM-2-0-FIRE-IoT-Kit-Dual-Core-ESP32-16M-FLash-4M-PSRAM-Development/3226069_32847906756.html?spm=2114.12010615.8148356.14.11c127aeBNzJBb)
| esp32/m5stick_c | [M5Stick-C](https://www.adafruit.com/product/4290)
| esp32/oddwires | [oddWires QVGA TFT touch display](http://www.oddwires.com/2-4-tft-touch-display-qvga/)
| gecko/blue | [SiLabs Blue Gecko](https://www.silabs.com/products/development-tools/wireless/bluetooth/blue-gecko-bluetooth-low-energy-soc-starter-kit)
| gecko/giant | [SiLabs Giant Gecko](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-starter-kit)
| gecko/mighty | [SiLabs Mighty Gecko](https://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit)
| gecko/thunderboard2 | [SiLabs Thunderboard Sense 2](https://www.silabs.com/products/development-tools/thunderboard/thunderboard-sense-two-kit)
| qca4020/cdb | [Qualcomm QCA4020 CDB](https://developer.qualcomm.com/hardware/qca4020-qca4024)

<a id="mac"></a>
## macOS

<a id="host-mac"></a>
### Host environment setup

> The Moddable SDK requires macOS Sierra version 10.12 or newer and Xcode version 9 or newer.

1. Download and install [Xcode](https://developer.apple.com/xcode/). Launch Xcode to install additional command line components when prompted.

2. Create a `Projects` directory in your home directory at `~/Projects` for the Moddable SDK repository.

	> Note: The Moddable SDK repository can be downloaded to any directory. These setup instructions assume the Moddable SDK is downloaded to the `~/Projects` directory.
	
3. Download the [Moddable repository](https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```
	cd ~/Projects
	git clone https://github.com/Moddable-OpenSource/moddable
	```

4. Setup the `MODDABLE` environment variable to point at your local Moddable SDK repository directory and edit the `PATH` environment variable in your `~/.profile` to include the build directory:

	```
	export MODDABLE="/Users/<user>/Projects/moddable"
	export PATH="${MODDABLE}/build/bin/mac/release:$PATH"
	```
	
	> Note: These instructions assume that your shell sources from `~/.profile` when a new terminal is opened. That may not be the case depending on what shell you use and how you have it configured. Starting with macOS Catalina, the [default shell is `zsh`](https://support.apple.com/en-us/HT208050) which uses `~/.zshrc` instead.
	
5. Build the Moddable command line tools, simulator, and debugger from the command line:

	```
	cd ${MODDABLE}/build/makefiles/mac
	make
	```
	
6. Launch the `xsbug` debugger from the command line:

	```
	open ${MODDABLE}/build/bin/mac/release/xsbug.app
	```

7. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p mac
	```

<a id="esp8266-mac"></a>
### ESP8266 setup

1. Complete ["Host environment setup"](#host-mac) for macOS.

2. Create an `esp` directory in your home directory at `~/esp` for required third party SDKs and tools.
 
3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

4. Download and untar the [Xtensa lx106 architecture GCC toolchain](http://www.moddable.tech/private/esp8266.toolchain.darwin.tgz). Copy the `toolchain` directory into the `~/esp` directory.

5. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `~/esp` directory.

6. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```
	cd ~/esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```

	We need version 3.2:

	```
	cd ESP8266_RTOS_SDK
	git checkout release/v3.2
	```
	
7. Install Python and the required Python packages. We've used [brew](https://brew.sh/) and [pip](https://pypi.org/project/pip/) to install the additional components:

	```
	brew install python
	sudo easy_install pip
	pip install --user pyserial
	```
	
8. Connect the ESP8266 to your computer with a USB cable.

9. Verify the setup by building `helloworld` for the `esp` target:

	```
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p esp
	```

<a id="esp32-mac"></a>
### ESP32 setup

1. Complete ["Host environment setup"](#host-mac) for macOS.

2. Create an `esp32` directory in your home directory at `~/esp32` for required third party SDKs and tools. 

3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

4. Download and untar the [ESP32 GCC toolchain](https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz). Copy the extracted `xtensa-esp32-elf` directory into your `~/esp32` directory.

	> Note: The extracted `xtensa-esp32-elf` directory contains a subdirectory that is also called `xtensa-esp32-elf`. Be sure to copy the top level `xtensa-esp32-elf` directory, not the subdirectory with the same name.

5. Clone the v3.2.2 branch of the `ESP-IDF` GitHub repository into your `~/esp32` directory. Make sure to specify the `--recursive` option:

	```
	cd ~/esp32
	git clone -b v3.2.2 --recursive https://github.com/espressif/esp-idf.git
	```

6. Set the `IDF_PATH` environment variable in your `~/.profile` to the `esp-idf` directory:

	```
	export IDF_PATH="/Users/<user>/esp32/esp-idf"
	```

	> Note: Close and reopen the Terminal window to enable the `IDF_PATH` environment variable.
	
7. Install Python and the `pip` package management system:

	```
	brew install python
	sudo easy_install pip
	```
	
8. Install the required Python packages:

	```
	python -m pip install --user -r $IDF_PATH/requirements.txt
	```

9. Update the `PATH` environment variable in your `~/.profile` to include the toolchain directory:

	```
	export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin
	```
		
10. Connect the ESP32 device to your macOS host with a USB cable and determine the serial port of the ESP32 device.

	To determine the serial port, examine the list of devices before and after plugging in your ESP32 device and note the new serial port that shows up. To see a list of serial devices, use the following command in Terminal:
	
	```
	ls /dev/cu.*
	```

11. Set the `UPLOAD_PORT` environment variable in your `~/.profile` to the ESP32 serial port:

	```
	export UPLOAD_PORT=/dev/cu.SLAB_USBtoUART
	```

	> Note the UPLOAD_PORT can also be specified on the `mcconfig` command line (see below), which can be useful when deploying to multiple ESP32 devices:
	
	```
	UPLOAD_PORT=/dev/cu.SLAB_USBtoUART mcconfig -d -m -p esp32
	```
	
12. Verify the setup by building `helloworld` for the `esp32` target:


	```
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p esp32
	```
	
	> Note that the first time you build an application for the ESP32 target, the toolchain may prompt you to enter configuration options. If this happens, accept the defaults.

	> If you encounter SSL certificate errors while building the ESP-IDF, you may need to install Python 2.7 and the required packages manually. We've used [brew](https://brew.sh/) and [pip](https://pypi.org/project/pip/) to install the additional components:
	>
	```
	brew install python
	brew install python@2
	pip install future
	pip install pyserial
	pip install cryptography
	```

<a id="windows"></a>
## Windows

<a id="host-windows"></a>
#### Host environment setup

> The Moddable SDK requires Windows 7 Pro SP1 or newer and Microsoft Visual Studio Community 2017 or newer.

1. Download [Microsoft Visual Studio 2019 Community Edition installer](https://www.visualstudio.com/downloads/). Launch the installer, choose the "Desktop development for C++" option, and install. 

2. Create a `Projects` directory in your `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>` for the Moddable SDK repository.

	> Note: The Moddable SDK repository can be downloaded to any directory. These setup instructions assume the Moddable SDK is downloaded to the `%USERPROFILE%` directory.

3. Download the [Moddable repository](https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```
	cd C:\Users\<user>\Projects
	git clone https://github.com/Moddable-OpenSource/moddable
	```

4. Setup the `MODDABLE` environment variable to point at your local Moddable SDK repository directory:

	```
	set MODDABLE=C:\Users\<user>\Projects\moddable
	```
	
5. Edit the system `PATH` environment variable to include the build directory:

	```
	%MODDABLE%\build\bin\win\release
	```
	
	> Environment variables should be set from the System Control Panel. The steps required vary depending on the Windows OS version.
	
6. Launch the "x86 Native Tools Command Prompt for VS 2019" command line console. Build the Moddable command line tools, simulator, and debugger from the command line:

	```
	cd %MODDABLE%\build\makefiles\win
	build
	```
	
7. Launch the `xsbug` debugger from the command line:

	```
	xsbug
	```
	
8. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p win
	```

<a id="esp8266-windows"></a>
### ESP8266 setup

1. Complete ["Host environment setup"](#host-windows) for Windows.

2. Create an `esp` directory in your home `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>`.
 
3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

4. Download the [esptool](https://github.com/igrr/esptool-ck/releases/download/0.4.13/esptool-0.4.13-win32.zip). Unzip the archive and copy the `esptool.exe` executable from the `esptool-0.4.13-win32` directory into the `esp` directory.

5. Download and unzip the [Cygwin toolchain support package](http://www.moddable.tech/private/cygwin.win32.zip). Copy the `cygwin` directory into the `esp` directory.
	
6. Download and unzip the [Xtensa lx106 architecture GCC toolchain](http://www.moddable.tech/private/esp8266.toolchain.win32.zip). Copy the `xtensa-lx106-elf` directory into the `esp` directory.

7. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `esp` directory.

8. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```
	cd C:\Users\<user>\esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```

	We need version 3.2:

	```
	cd ESP8266_RTOS_SDK
	git checkout release/v3.2
	```

9. Download and run the [Python installer](https://www.python.org/ftp/python/2.7.15/python-2.7.15.msi) for Windows. Choose the default options.

10. Edit the system `PATH` environment variable to include the Python directories:

	```
	C:\Python27
	C:\Python27\Scripts
	```

11. Open a "Command Prompt" window and install the `pyserial` Python Serial Port Extension:

	```
	pip install pyserial
	```
	
12. Connect the ESP8266 to your computer with a USB cable.

13. Launch the Windows Device Manager, open the "Ports (COM & LPT)" section, and verify the "Silicon Labs CP210x USB to UART Bridge" is displayed. Note the COM port (e.g. COM3) for the next step.

	> The Device Manager interface may vary depending on the Windows OS version.
	
14. Set the `BASE_DIR`, `UPLOAD_PORT` and `SERIAL2XSBUG` Windows environment variables to your `%USERPROFILE%` directory, device COM port and serial2xsbug.exe tool path. Note that forward slashes are required in the tool path:

	```
	set BASE_DIR=%USERPROFILE%
	set UPLOAD_PORT=COM3
	set SERIAL2XSBUG=/c/Users/<your-user-name>/Projects/moddable/build/bin/win/release/serial2xsbug.exe
	```


15. Edit the system `PATH` environment variable to include the `cygwin\bin` directory:

	```
	%BASE_DIR%\esp\cygwin\bin
	```

14. Launch the "x86 Native Tools Command Prompt for VS 2019" command line console. Verify the setup by building `helloworld` for the `esp` target:

	```
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p esp
	```

<a id="esp32-windows"></a>	
### ESP32 setup

1. Complete ["Host environment setup"](#host-windows) for Windows.

2. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

3. Download the Espressif [all-in-one Windows toolchain and MSYS2](https://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20181001.zip) zip archive. Copy the extracted `msys32` directory into your home `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>\msys32`.

4. Open a MSYS2 MINGW32 terminal window from a Windows command line console:

	```
	%USERPROFILE%\msys32\mingw32.exe
	```
	
5. From the MINGW32 terminal window, create an `esp` directory in the home `~` directory:

	```
	mkdir ~/esp
	```
	
6. Clone the v3.2.2 branch of the `ESP-IDF` GitHub repository into your `~/esp` directory. Make sure to specify the `--recursive` option:

	```
	cd ~/esp
	git clone -b v3.2.2 --recursive https://github.com/espressif/esp-idf.git
	```

7. Create a new script file `esp32_moddable.sh` in your `%USERPROFILE%\msys32\etc\profile.d\` directory. Add an export command for the ESP-IDF path to the script file:

	```
	export IDF_PATH="C:/Users/<your-user-name>/msys32/home/<your-user-name>/esp/esp-idf"
	```

8. Connect the ESP32 to your computer with a USB cable.

9. Launch the Windows Device Manager, open the "Ports (COM & LPT)" section, and verify the "Silicon Labs CP210x USB to UART Bridge" is displayed. Note the COM port (e.g. COM3) for the next step.

	> The Device Manager interface may vary depending on the Windows OS version.
	
10. Set the `BASE_DIR`, `UPLOAD_PORT` and `SERIAL2XSBUG` Windows environment variables to your `%USERPROFILE%` directory, device COM port and serial2xsbug.exe tool path. Note that forward slashes are required in the tool path:

	```
	set BASE_DIR=%USERPROFILE%
	set UPLOAD_PORT=COM3
	set SERIAL2XSBUG=/c/Users/<your-user-name>/Projects/moddable/build/bin/win/release/serial2xsbug.exe
	```

11. Launch the "x86 Native Tools Command Prompt for VS 2019" command line console. Verify the setup by building `helloworld` for the `esp32` target:

	```
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p esp32
	```
	
	> The mcconfig tool launches a MINGW32 shell to configure the ESP32 firmware build. After this configuration completes, the MINGW32 shell closes and control is returned back to the command line console. Press any key to complete the build and Flash the binary to the device. Another MINGW32 shell opens to complete the build.
	
	> Note that the first time you build an application for the ESP32 target, the toolchain may prompt you to enter configuration options. If this happens, accept the defaults.	

<a id="linux"></a>
## Linux

<a id="host-linux"></a>
### Host environment setup

> The Moddable SDK has been tested on the Ubuntu 16.04 LTS (64-bit) and Raspberry Pi Desktop (32-bit) operating systems. These setup instructions assume that a GCC toolchain has already been installed.

1. Install the development version of the GTK+ 3 library:

	```
	sudo apt-get install libgtk-3-dev
	```
	
2. Create a `Projects` directory in your home directory at `~/Projects` for the Moddable SDK repository.

	> Note: The Moddable SDK repository can be downloaded to any directory. These setup instructions assume the Moddable SDK is downloaded to the `~/Projects` directory.

3. Download the [Moddable repository](https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```
	cd ~/Projects
	git clone https://github.com/Moddable-OpenSource/moddable
	```

4. Setup the `MODDABLE` environment variable in your `~/.bashrc` file to point at your local Moddable SDK repository directory:

	```
	MODDABLE=~/Projects/moddable
	export MODDABLE
	```
	
5. Build the Moddable command line tools, simulator, and debugger from the command line:

	```
	cd $MODDABLE/build/makefiles/lin
	make
	```
	
6. Update the `PATH` environment variable in your `~/.bashrc` to include the tools directory:

	```
	export PATH=$PATH:$MODDABLE/build/bin/lin/release
	```

7. Install the Screen Test desktop simulator and `xsbug` debugger applications:

	```
	cd $MODDABLE/build/makefiles/lin
	make install
	```

	When prompted, enter your `sudo` password to copy the application's desktop, executable and icon files into the standard `/usr/share/applications`, `/usr/bin`, and `/usr/share/icon/hicolor` directories.
	
8. Launch the `xsbug` debugger from the command line:

	```
	xsbug
	```
	
9. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p lin
	```

<a id="esp8266-linux"></a>
### ESP8266 setup

1. Complete ["Host environment setup"](#host-linux) for Linux.

2. Create an `esp` directory in your home directory at `~/esp` for required third party SDKs and tools.
 
3. Download and untar the [Xtensa lx106 architecture GCC toolchain](http://www.moddable.tech/private/esp8266.toolchain.linux.tgz). Copy the `toolchain` directory into the `~/esp` directory.

4. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `~/esp` directory.

5. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```
	cd ~/esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```

	We need version 3.2:

	```
	cd ESP8266_RTOS_SDK
	git checkout release/v3.2
	```

6. Install Python and the required Python packages. We've used [brew](https://brew.sh/) and [pip](https://pypi.org/project/pip/) to install the additional components:

	```
	brew install python
	sudo easy_install pip
	pip install --user pyserial
	```
	
7. Connect the ESP8266 to your computer with a USB cable.

8. Verify the setup by building `helloworld` for the `esp` target:

	```
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p esp
	```

	> The ESP8266 communicates with the Linux host via the ttyUSB0 device. On Ubuntu Linux the ttyUSB0 device is owned by the `dialout` group. If you get a **permission denied error** when flashing the ESP8266, add your user to the `dialout` group:
	> 
	```
	sudo adduser <username> dialout 
	sudo reboot
	```

<a id="esp32-linux"></a>
### ESP32 setup

1. Complete ["Host environment setup"](#host-linux) for Linux.

2. Create an `esp32` directory in your home directory at `~/esp32` for required third party SDKs and tools. 

3. Download and untar the [64-bit](https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz) or [32-bit](https://dl.espressif.com/dl/xtensa-esp32-elf-linux32-1.22.0-80-g6c4433a-5.2.0.tar.gz) ESP32 GCC toolchain compatible with your Linux host. Copy the extracted `xtensa-esp32-elf` directory into your `~/esp32` directory.

	> Note: The extracted `xtensa-esp32-elf` directory contains a subdirectory that is also called `xtensa-esp32-elf`. Be sure to copy the top level `xtensa-esp32-elf` directory, not the subdirectory with the same name.

4. Clone the v3.2.2 branch of the `ESP-IDF` GitHub repository into your `~/esp32` directory. Make sure to specify the `--recursive` option:

	```
	cd ~/esp32
	git clone -b v3.2.2 --recursive https://github.com/espressif/esp-idf.git
	```

5. Install the packages required to compile with the `ESP-IDF`:

	```
	sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-pip python-setuptools python-serial 
	```
	
6. Set the `IDF_PATH` environment variable in your `~/.bashrc` to the `esp-idf` directory:

	```
	IDF_PATH=~/esp32/esp-idf
	export IDF_PATH
	```

7. Install the Python `pip` package management system:

	```
	cd ~/esp32
	sudo easy_install pip
	```
	
8. Install the required Python packages:

	```
	python -m pip install --user -r $IDF_PATH/requirements.txt
	```

9. Update the `PATH` environment variable in your `~/.bashrc` to include the toolchain directory:

	```
	export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin
	```
		
10. Connect the ESP32 device to your Linux host with a USB cable.

11. Determine the USB device path used by the ESP32 device, e.g. `/dev/ttyUSB0`:

	```
	ls /dev
	```
	
12. Set the `UPLOAD_PORT` environment variable in your `~/.bashrc` to the ESP32 serial port:

	```
	UPLOAD_PORT=/dev/ttyUSB0
	export UPLOAD_PORT
	```

13. Verify the setup by building `helloworld` for the `esp32` target:

	```
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p esp32
	```

	For Moddable Two applications that rely on the screen or use I2C pins, build for the `esp32` target with the `moddable_two` subplatform.

	```
	cd ${MODDABLE}/examples/piu/balls
	mcconfig -d -m -p esp32/moddable_two
	```

	> The ESP32 communicates with the Linux host via the ttyUSB0 device. On Ubuntu Linux the ttyUSB0 device is owned by the `dialout` group. If you get a **permission denied error** when flashing the ESP32, add your user to the `dialout` group:
	> 
	```
	sudo adduser <username> dialout 
	sudo reboot
	```

	> Note that the first time you build an application for the ESP32 target, the toolchain may prompt you to enter configuration options. If this happens, accept the defaults.

<a id="debugging-applications"></a>
## Debugging applications

The `xsbug` JavaScript source level debugger is built as part of the Moddable SDK build described above. `xsbug` is a full featured debugger that supports debugging modules and applications for [XS platforms](xs/XS%20Platforms.md). The `xsbug` debugger is automatically launched when deploying debug builds and connects to devices via USB or over Wi-Fi. Similar to other debuggers, `xsbug` supports setting breakpoints, browsing source code, the call stack and variables. The `xsbug` debugger additionally provides real-time instrumentation to track memory usage and profile application and resource consumption.

For additional details on `xsbug` please refer to the [xsbug](xs/xsbug.md) document.

<a id="arduino-version"></a>
## ESP8266 Arduino version 2.4

The Moddable SDK on ESP8266 is hosted by the [ESP8266 core for Arduino](https://github.com/esp8266/Arduino). The Moddable SDK uses version 2.3. Version 2.4 is supported as well. At this time, we do not recommend using version 2.4 as it requires more ROM and more RAM without providing significant benefits for most uses of the Moddable SDK. The team responsible for ESP8266 core for Arduino is aware of [these](https://github.com/esp8266/Arduino/issues/3740) [issues](https://github.com/esp8266/Arduino/issues/4089) and actively working to address them.

You can use version 2.4 today if building on macOS or Linux. 

- Follow the instructions above, but use the [version 2.4](https://github.com/esp8266/Arduino/releases/download/2.4.0/esp8266-2.4.0.zip) of ESP8266 Core for Arduino.

- In `${MODDABLE}/tools/mcconfig/make.esp.mk` change:

	 ```
	 ESP_SDK_RELEASE ?= esp8266-2.3.0
	 ```
	 
	 to:
	 
	 ```
	 ESP_SDK_RELEASE ?= esp8266-2.4.0
	 ```

- Do a clean build of tools by deleting the contents of `${MODDABLE}/build/bin/esp/` and `${MODDABLE}/build/tmp/esp/` and building as above.
