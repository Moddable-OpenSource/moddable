# Moddable SDK – Getting Started
Copyright 2016-2020 Moddable Tech, Inc.<BR>
Revised: September 1, 2020

This document provides an introduction to getting started building apps with the Moddable SDK. It describes how to configure the host build environments, install the required SDKs, drivers and development tools, build applications, and use `xsbug`, the JavaScript source code debugger.

If you have already set up your host environment and target platform builds once, use the [Keeping Up To Date](Moddable%20SDK%20-%20Keeping%20Up%20To%20Date.md) document to keep your Moddable SDK tools and build environment up to date over time.

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
* [Troubleshooting](#troubleshooting)
* [Debugging applications](#debugging-applications)
* [ESP8266 Arduino version 2.4](#arduino-version)

<a id="overview"></a>
## Overview

Before you can build applications, you need to set up your host environment and install the required drivers and development tools for your target platform.

The instructions below will have you verify your setup by running the `helloworld` example on your target platform using `mcconfig`, a command line tool that builds and runs Moddable applications. You use a platform identifier to specify which target `mcconfig` should build for. This document contains instructions for the ESP8266 and ESP32 platforms, which use the platform identifiers `esp` and `esp32`, respectively. For example:

```text
mcconfig -d -m -p esp
```

To build for other ESP8266 or ESP32-based devices--for example the Moddable One and Moddable Two--you must also specify a subplatform. For example:

```text
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

> The Moddable SDK requires macOS Sierra version 10.12 or newer and a full installation of Xcode version 9 or newer.

1. Download and install [Xcode](https://developer.apple.com/xcode/). Launch Xcode to install additional command line components when prompted.

2. Create a `Projects` directory in your home directory at `~/Projects` for the Moddable SDK repository.

	> Note: The Moddable SDK repository can be downloaded to any directory. These setup instructions assume the Moddable SDK is downloaded to the `~/Projects` directory.
	
3. Download the [Moddable repository](https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```text
	cd ~/Projects
	git clone https://github.com/Moddable-OpenSource/moddable
	```

4. Setup the `MODDABLE` environment variable to point at your local Moddable SDK repository directory and edit the `PATH` environment variable in your `~/.profile` to include the build directory:

	```text
	export MODDABLE="/Users/<user>/Projects/moddable"
	export PATH="${MODDABLE}/build/bin/mac/release:$PATH"
	```
	
	> Note: These instructions assume that your shell sources from `~/.profile` when a new terminal is opened. That may not be the case depending on what shell you use and how you have it configured. Starting with macOS Catalina, the [default shell is `zsh`](https://support.apple.com/en-us/HT208050) which uses `~/.zshrc` instead.

	> Note: You must open a new shell instance or manually run the export statements in your shell before proceeding. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.
	
5. Build the Moddable command line tools, simulator, and debugger from the command line:

	```text
	cd ${MODDABLE}/build/makefiles/mac
	make
	```
	
6. Launch the `xsbug` debugger from the command line:

	```text
	open ${MODDABLE}/build/bin/mac/release/xsbug.app
	```

7. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```text
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p mac
	```

**Troubleshooting:**
 - If the Moddable SDK build fails with an error like "`xcode-select: error: tool 'ibtool' requires Xcode, but active developer directory '/Library/Developer/CommandLineTools' is a command line tools instance.`" there are three potential issues and fixes:
     1. You may have a command line tools-only Xcode installation. The Moddable SDK build requires a full installation of Xcode. 
     2. You may need to launch the Xcode application to accept Xcode's license agreement and install the command line components. 
     3. If you have a full Xcode installation but your build fails with this error, you need to use the `xcode-select` utility to select your full Xcode installation. This can be done with this command, with the path adjusted as necessary for your environment: `sudo xcode-select -s /Applications/Xcode.app/Contents/Developer`

<a id="esp8266-mac"></a>
### ESP8266 setup

1. Complete ["Host environment setup"](#host-mac) for macOS.

2. Create an `esp` directory in your home directory at `~/esp` for required third party SDKs and tools.
 
3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

	> Note: If you run macOS Catalina, an extra step is required to enable the VCP driver. If you see a popup that says "System Extension Blocked" during installation, follow the instructions in the dialog to enable the extension in Security & Privacy System Preferences.

4. If you use macOS Catalina (version 10.15), add an exemption to allow Terminal (or your alternate terminal application of choice) to run software locally that does not meet the system's security policy. Without this setting, the precompiled Xtensa toolchain you will download in the next step will not be permitted to run. 

	To set the security policy exemption for Terminal, go into the Security & Privacy System Preferences, select the Privacy tab, choose Developer Tools from the list on the left, and then tick the checkbox for Terminal or the alternate terminal application from which you will be building Moddable SDK apps. The end result should look like this:

	![Catalina Developer Options](./assets/getting-started/catalina-security.png)

5. Download and untar the [Xtensa lx106 architecture GCC toolchain](https://www.moddable.com/private/esp8266.toolchain.darwin.tgz). Copy the `toolchain` directory into the `~/esp` directory.

6. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `~/esp` directory.

7. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```text
	cd ~/esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```

	We need version 3.2:

	```text
	cd ESP8266_RTOS_SDK
	git checkout release/v3.2
	```
	
8. Install Python and the required Python packages. We've used [brew](https://brew.sh/) and [pip](https://pypi.org/project/pip/) to install the additional components:

	```text
	brew install python
	sudo easy_install pip
	pip install --user pyserial
	```
	
9. Connect the ESP8266 to your computer with a USB cable.

10. Verify the setup by building `helloworld` for the `esp` target:

	```text
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p esp
	```

<a id="esp32-mac"></a>
### ESP32 setup

1. Complete ["Host environment setup"](#host-mac) for macOS.

2. Create an `esp32` directory in your home directory at `~/esp32` for required third party SDKs and tools. 

3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

	> Note: If you run macOS Catalina, an extra step is required to enable the VCP driver. If you see a popup that says "System Extension Blocked" during installation, follow the instructions in the dialog to enable the extension in Security & Privacy System Preferences.

4. Download and untar the [ESP32 GCC toolchain](https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz). Copy the extracted `xtensa-esp32-elf` directory into your `~/esp32` directory.

	Note that the extracted `xtensa-esp32-elf` directory contains a subdirectory that is also called `xtensa-esp32-elf`. Be sure to copy the top level `xtensa-esp32-elf` directory, not the subdirectory with the same name. Your directory tree structure should look like this:
	
	```text
	~/esp32
	├── xtensa-esp32-elf
	│   ├── bin
	│   ├── include
	│   ├── lib
	│   ├── libexec
	│   ├── share
	│   └── xtensa-esp32-elf
	```

5. Clone the v3.3.2 branch of the `ESP-IDF` GitHub repository into your `~/esp32` directory. Make sure to specify the `--recursive` option:

	```text
	cd ~/esp32
	git clone -b v3.3.2 --recursive https://github.com/espressif/esp-idf.git
	```

	> Note: If you already have a cloned copy of the ESP-IDF, you can update it in place by fetching updated sources and selecting the v3.3.2 tag:

    ```text
    cd ~/esp32/esp-idf
    git fetch
    git checkout v3.3.2
    git submodule update
    ```

6. Update homebrew and then install Python, cmake, ninja, and the pip package management system. Also run a `brew upgrade` on those packages, in case you already had older versions installed:

	```text
	brew update
	brew install python cmake ninja
	brew upgrade python cmake ninja
	sudo easy_install pip
	```
	
7. Install required Python packages:

	```text
	python -m pip install --user -r ~/esp32/esp-idf/requirements.txt
	```

8. Connect the ESP32 device to your macOS host with a USB cable.
		
9. Set the `IDF_PATH` and `PATH` environment variables in your shell's user profile file (e.g. `~/.profile` or `~/.zshrc`, depending on your shell). Update the paths for your system.

	```
	export IDF_PATH=$HOME/esp32/esp-idf
	export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin:$IDF_PATH/tools
	```

	There are two optional environment variables for advanced users: `UPLOAD_PORT` and `ESP32_CMAKE`.<br><br>
	The ESP-IDF build/config tool `idf.py` automatically detects the serial port in most cases. If it does not, set the path of the port to use in the `UPLOAD_PORT` environment variable.

	```
	export UPLOAD_PORT=/dev/cu.SLAB_USBtoUART
	```

	To identify the proper serial port, examine the list of serial devices in macOS before and after plugging in your ESP32 device and note the new serial port that shows up. To see a list of serial device files, use the following command in Terminal:
	
	```text
	ls /dev/cu.*
	```

	The `UPLOAD_PORT` can also be specified on the `mcconfig` command line, which is useful when deploying to multiple ESP32 devices.
	
	```
	UPLOAD_PORT=/dev/cu.SLAB_USBtoUART mcconfig -d -m -p esp32
	```

	The `ESP32_CMAKE` environment variable controls whether the ESP-IDF is built using the newer CMake or older `make`-based tools. The default is 1, which builds with CMake. Set `ESP32_CMAKE` to 0 to use the older `make`-based build. Support for `make`-based builds will be removed in a future Moddable SDK update.

	> Note: You must open a new shell instance or manually run the export statements in your shell before proceeding. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.

10. Verify the setup by building `helloworld` for the `esp32` target:


	```text
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p esp32
	```
	
	> Note that the first time you build an application for the ESP32 target, the toolchain may prompt you to enter configuration options. If this happens, accept the defaults.

	> If you encounter SSL certificate errors while building the ESP-IDF, you may need to install Python 2.7 and the required packages manually. We've used [brew](https://brew.sh/) and [pip](https://pypi.org/project/pip/) to install the additional components:
	>
	```text
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

> The Moddable SDK requires Windows 8.1 or newer and Microsoft Visual Studio Community 2017 or newer.

1. Download [Microsoft Visual Studio 2019 Community Edition installer](https://www.visualstudio.com/downloads/). Launch the installer, choose the "Desktop development for C++" option, and install. 

2. Create a `Projects` directory in your `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>` for the Moddable SDK repository.

	> Note: The Moddable SDK repository can be downloaded to any directory. These setup instructions assume the Moddable SDK is downloaded to the `%USERPROFILE%` directory.

3. Download the [Moddable repository](https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```text
	cd C:\Users\<user>\Projects
	git clone https://github.com/Moddable-OpenSource/moddable
	```

4. Set the `MODDABLE` environment variable to point at your local Moddable SDK repository directory:

	```text
	set MODDABLE=C:\Users\<user>\Projects\moddable
	```
	
5. Edit the system `PATH` environment variable to include the build directory:

	```text
	%MODDABLE%\build\bin\win\release
	```
	
	> Environment variables should be set from the System Control Panel. The steps required vary depending on the Windows OS version.
	
6. Launch the "x86 Native Tools Command Prompt for VS 2019" command line console. Build the Moddable command line tools, simulator, and debugger from the command line:

	```text
	cd %MODDABLE%\build\makefiles\win
	build
	```
	
7. Launch the `xsbug` debugger from the command line:

	```text
	xsbug
	```
	
8. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```text
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p win
	```

**Troubleshooting:**
 - If the Moddable SDK build fails with an error like "`LINK : fatal error LNK1104: cannot open file '<Moddable path>\build\bin\win\release\<tool name>.exe'`", it most likely indicates a conflict with antivirus software as described in [this document from Microsoft](https://docs.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-error-lnk1104?view=vs-2019). Please try excluding the `%MODDABLE%` directory from your antivirus software's live scanning capability during the build process.

<a id="esp8266-windows"></a>
### ESP8266 setup

1. Complete ["Host environment setup"](#host-windows) for Windows.

2. Create an `esp` directory in your home `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>`.
 
3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers). The driver zip file contains x64 and x86 versions of the installer. Most modern PCs run 64-bit Windows and should use the x64 version of the VCP driver. If you run a 32-bit version of Windows, use the x86 version of the driver. (You can determine if your computer is running a 64-bit version of Windows by checking "About your PC" in System Settings.) 

4. Download the [esptool](https://github.com/igrr/esptool-ck/releases/download/0.4.13/esptool-0.4.13-win32.zip). Unzip the archive and copy the `esptool.exe` executable from the `esptool-0.4.13-win32` directory into the `esp` directory.

5. Download and unzip the [Cygwin toolchain support package](https://www.moddable.com/private/cygwin.win32.zip). Copy the `cygwin` directory into the `esp` directory.
	
6. Download and unzip the [Xtensa lx106 architecture GCC toolchain](https://www.moddable.com/private/esp8266.toolchain.win32.zip). Copy the `xtensa-lx106-elf` directory into the `esp` directory.

7. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `esp` directory.

8. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```text
	cd C:\Users\<user>\esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```

	We need version 3.2:

	```text
	cd ESP8266_RTOS_SDK
	git checkout release/v3.2
	```

9. Download and run the [Python installer](https://www.python.org/ftp/python/2.7.15/python-2.7.15.msi) for Windows. Choose the default options.

10. Edit the system `PATH` environment variable to include the Python directories:

	```text
	C:\Python27
	C:\Python27\Scripts
	```

11. Open a "Command Prompt" window and install the `pyserial` Python Serial Port Extension:

	```text
	pip install pyserial
	```
	
12. Connect the ESP8266 to your computer with a USB cable.

13. Launch the Windows Device Manager, open the "Ports (COM & LPT)" section, and verify the "Silicon Labs CP210x USB to UART Bridge" is displayed. Note the COM port (e.g. COM3) for the next step.

	> The Device Manager interface may vary depending on the Windows OS version.
	
14. Set the `BASE_DIR` and `UPLOAD_PORT` environment variables to your `%USERPROFILE%` directory and device COM port:

	```text
	set BASE_DIR=%USERPROFILE%
	set UPLOAD_PORT=COM3
	```


15. Edit the system `PATH` environment variable to include the `cygwin\bin` directory:

	```text
	%BASE_DIR%\esp\cygwin\bin
	```

14. Launch the "x86 Native Tools Command Prompt for VS 2019" command line console. Verify the setup by building `helloworld` for the `esp` target:

	```text
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p esp
	```

<a id="esp32-windows"></a>	
### ESP32 setup

1. Complete ["Host environment setup"](#host-windows) for Windows.

2. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers). The driver zip file contains x64 and x86 versions of the installer. Most modern PCs run 64-bit Windows and should use the x64 version of the VCP driver. If you run a 32-bit version of Windows, use the x86 version of the driver. (You can determine if your computer is running a 64-bit version of Windows by checking "About your PC" in System Settings.)

3. Download and run the Espressif [ESP-IDF Tools Installer](https://dl.espressif.com/dl/esp-idf-tools-setup-1.2.exe). This will install the ESP32 Xtensa gcc toolchain, Ninja Build, and a Windows-based kconfig tool. This tool will also set your `PATH` to include the newly downloaded tools, as necessary.

    It is safe to accept all of the default options in the installer, or to change install locations as necessary.

    If you do not already have CMake or Python 2.7, the installer will also prompt you to download and install those tools (you should do so if needed).

4. Create an `esp32` directory in your home folder, either from File Explorer or a terminal. For instance, in Git Bash:

    ```text
    cd ~
    mkdir esp32
    ```

5. Clone the v3.3.2 branch of the `ESP-IDF` Github repository into your `~/esp32` directory. Make sure to specify the `--recursive` option:

    ```text
    cd ~/esp32
    git clone -b v3.3.2 --recursive https://github.com/espressif/esp-idf.git
    ```

	> Note: If you already have a cloned copy of the ESP-IDF, you can update it in place by fetching updated sources and selecting the v3.3.2 tag:

    ```text
    cd ~/esp32/esp-idf
    git fetch
    git checkout v3.3.2
    git submodule update
    ```

6. Connect the ESP32 device to your Windows host with a USB cable.
	
7. Set the `IDF_PATH` and `PATH` Windows environment variables. Update the paths for your system and remember to open a new shell instance to pick up these changes before proceeding. Setting environment variables in Windows is generally done [through System Properties](https://www.architectryan.com/2018/08/31/how-to-change-environment-variables-on-windows-10/).

    - `IDF_PATH`: the directory where you cloned the ESP-IDF, e.g. `%userprofile%\esp32\esp-idf`
    - `PATH`: Add the directory `%IDF_PATH%\tools` to your `PATH`.

	There are two optional environment variables for advanced users: `UPLOAD_PORT` and `ESP32_CMAKE`.<br><br>
	The ESP-IDF build/config tool `idf.py` automatically detects the serial port in most cases. If it does not, set the path of the port to use in the `UPLOAD_PORT` environment variable.

    - `UPLOAD_PORT`: the COM port for your device, e.g. `COM3`

	To identify the correct serial port, launch the Windows Device Manager. Open the "Ports (COM & LPT)" section, verify the "Silicon Labs CP210x USB to UART Bridge" is displayed, and note the associated COM port (e.g. COM3).

	The `ESP32_CMAKE` environment variable controls whether the ESP-IDF is built using the newer CMake or older `MinGW`-based tools. The default is 1, which builds with CMake. Set `ESP32_CMAKE` to 0 to use the older `MinGW`-based build. Support for `MinGW`-based builds will be removed in a future Moddable SDK update.

8. Launch the "x86 Native Tools Command Prompt for VS 2019" command line console. Verify the setup by building `helloworld` for the `esp32` target:

	```text
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p esp32
	```

**Troubleshooting:**
 - If you get an error about Python dependencies not being installed, it means that the ESP-IDF installer failed to update Python. This usually happens due to permissions issues on your machine. To correct it, run `python -m pip install --user -r %IDF_PATH%\requirements.txt` from the "x86 Native Tools Command Prompt for VS 2019."	

<a id="linux"></a>
## Linux

<a id="host-linux"></a>
### Host environment setup

> The Moddable SDK has been tested on the Ubuntu 16.04 LTS (64-bit) and Raspberry Pi Desktop (32-bit) operating systems. These setup instructions assume that a GCC toolchain has already been installed.

1. Install the development version of the GTK+ 3 library:

	```text
	sudo apt-get install libgtk-3-dev
	```
	
2. Create a `Projects` directory in your home directory at `~/Projects` for the Moddable SDK repository.

	> Note: The Moddable SDK repository can be downloaded to any directory. These setup instructions assume the Moddable SDK is downloaded to the `~/Projects` directory.

3. Download the [Moddable repository](https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```text
	cd ~/Projects
	git clone https://github.com/Moddable-OpenSource/moddable
	```

4. Setup the `MODDABLE` environment variable in your `~/.bashrc` file to point at your local Moddable SDK repository directory:

	```text
	MODDABLE=~/Projects/moddable
	export MODDABLE
	```

	> Note: You must open a new shell instance or manually run the export statements in your shell before proceeding. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.

5. Build the Moddable command line tools, simulator, and debugger from the command line:

	```text
	cd $MODDABLE/build/makefiles/lin
	make
	```
	
6. Update the `PATH` environment variable in your `~/.bashrc` to include the tools directory:

	```text
	export PATH=$PATH:$MODDABLE/build/bin/lin/release
	```

	> Note: You must open a new shell instance or manually run the export statements in your shell before proceeding. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.

7. Install the Screen Test desktop simulator and `xsbug` debugger applications:

	```text
	cd $MODDABLE/build/makefiles/lin
	make install
	```

	When prompted, enter your `sudo` password to copy the application's desktop, executable and icon files into the standard `/usr/share/applications`, `/usr/bin`, and `/usr/share/icon/hicolor` directories.
	
8. Launch the `xsbug` debugger from the command line:

	```text
	xsbug
	```
	
9. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```text
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p lin
	```

<a id="esp8266-linux"></a>
### ESP8266 setup

1. Complete ["Host environment setup"](#host-linux) for Linux.

2. Create an `esp` directory in your home directory at `~/esp` for required third party SDKs and tools.
 
3. Download and untar the [Xtensa lx106 architecture GCC toolchain](https://www.moddable.com/private/esp8266.toolchain.linux.tgz). Copy the `toolchain` directory into the `~/esp` directory.

4. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `~/esp` directory.

5. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```text
	cd ~/esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```

	We need version 3.2:

	```text
	cd ESP8266_RTOS_SDK
	git checkout release/v3.2
	```

6. Install Python and the required Python packages. We've used [pip](https://pypi.org/project/pip/) to install the additional components. The packages to install vary based on your distribution's default Python version.

	For Ubuntu 20.04 or newer (and other Linux distributions that default to Python 3):

	```text
	sudo apt-get install python-is-python3 python3-pip python3-serial
	```

	For Ubuntu versions prior to 20.04 (and other Linux distributions that default to Python 2):

	```text
	sudo apt-get install python
	sudo easy_install pip
	pip install --user pyserial
	```
	
7. Connect the ESP8266 to your computer with a USB cable.

8. Verify the setup by building `helloworld` for the `esp` target:

	```text
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p esp
	```

	> The ESP8266 communicates with the Linux host via the ttyUSB0 device. On Ubuntu Linux the ttyUSB0 device is owned by the `dialout` group. If you get a **permission denied error** when flashing the ESP8266, add your user to the `dialout` group:
	> 
	```text
	sudo adduser <username> dialout 
	sudo reboot
	```

<a id="esp32-linux"></a>
### ESP32 setup

1. Complete ["Host environment setup"](#host-linux) for Linux.

2. Create an `esp32` directory in your home directory at `~/esp32` for required third party SDKs and tools. 

3. Download and untar the [64-bit](https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz) or [32-bit](https://dl.espressif.com/dl/xtensa-esp32-elf-linux32-1.22.0-80-g6c4433a-5.2.0.tar.gz) ESP32 GCC toolchain compatible with your Linux host. Copy the extracted `xtensa-esp32-elf` directory into your `~/esp32` directory.

	Note that the extracted `xtensa-esp32-elf` directory contains a subdirectory that is also called `xtensa-esp32-elf`. Be sure to copy the top level `xtensa-esp32-elf` directory, not the subdirectory with the same name. Your directory tree structure should look like this:
	
	```text
	~/esp32
	├── xtensa-esp32-elf
	│   ├── bin
	│   ├── include
	│   ├── lib
	│   ├── libexec
	│   ├── share
	│   └── xtensa-esp32-elf
	```
	
4. Clone the v3.3.2 branch of the `ESP-IDF` GitHub repository into your `~/esp32` directory. Make sure to specify the `--recursive` option:

	```text
	cd ~/esp32
	git clone -b v3.3.2 --recursive https://github.com/espressif/esp-idf.git
	```

	> Note: If you already have a cloned copy of the ESP-IDF, you can update it in place by fetching updated sources and selecting the v3.3.2 tag:

    ```text
    cd ~/esp32/esp-idf
    git fetch
    git checkout v3.3.2
    git submodule update
    ```

5. Install the packages required to compile with the `ESP-IDF`. The packages to install vary based on your distribution's default Python version.

	For Ubuntu 20.04 or newer (and other Linux distributions that default to Python 3):

	```text
	sudo apt-get install gcc git wget make libncurses-dev flex bison gperf cmake ninja-build python-is-python3 python3-pip python3-serial
	```

	For Ubuntu versions prior to 20.04 (and other Linux distributions that default to Python 2):

	```text
	sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-pip python-setuptools python-serial cmake ninja-build
	```

6. Connect the ESP32 device to your Linux host with a USB cable.

7. Set the `IDF_PATH` and `PATH` environment variables are set correctly in your shell's user profile file (e.g. `~/.profile` or `~/.zshrc`, depending on your shell). Update the paths for your system.

	```
    export IDF_PATH=~/esp32/esp-idf
	export PATH=$PATH:~/esp32/xtensa-esp32-elf/bin:$IDF_PATH/tools
	```

	There are two optional environment variables for advanced users: `UPLOAD_PORT` and `ESP32_CMAKE`.<br><br>
	The ESP-IDF build/config tool `idf.py` automatically detects the serial port in most cases. If it does not, set the path of the port to use in the `UPLOAD_PORT` environment variable.

	```
	export UPLOAD_PORT=/dev/ttyUSB0
	```

	To identify the proper serial port, examine the list of serial devices on your Linux host before and after plugging in your ESP32 device and note the new serial port that shows up. To see a list of serial device files, use the following command:
	
	```text
	ls /dev/*
	```

	The `UPLOAD_PORT` can also be specified on the `mcconfig` command line, which is useful when deploying to multiple ESP32 devices.
	
	```
	UPLOAD_PORT=/dev/ttyUSB0 mcconfig -d -m -p esp32
	```
	
	The `ESP32_CMAKE` environment variable controls whether the ESP-IDF is built using the newer CMake or older `make`-based tools. The default is 1, which builds with CMake. Set `ESP32_CMAKE` to 0 to use the older `make`-based build. Support for `make`-based builds will be removed in a future Moddable SDK update.

	> Note: You must open a new shell instance or manually run the export statements in your shell. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.

8. Install the required Python packages:

	```text
	python -m pip install --user -r $IDF_PATH/requirements.txt
	```

9. Verify the setup by building `helloworld` for the `esp32` target:

	```text
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p esp32
	```

	For Moddable Two applications that rely on the screen or use I2C pins, build for the `esp32` target with the `moddable_two` subplatform.

	```text
	cd ${MODDABLE}/examples/piu/balls
	mcconfig -d -m -p esp32/moddable_two
	```

	> The ESP32 communicates with the Linux host via the ttyUSB0 device. On Ubuntu Linux the ttyUSB0 device is owned by the `dialout` group. If you get a **permission denied error** when flashing the ESP32, add your user to the `dialout` group:
	> 
	```text
	sudo adduser <username> dialout 
	sudo reboot
	```

	> Note that the first time you build an application for the ESP32 target, the toolchain may prompt you to enter configuration options. If this happens, accept the defaults.

<a id="troubleshooting"></a>
## Troubleshooting

When you're trying to install applications, you may experience roadblocks in the form of errors or warnings; this section explains some common issues and how to resolve them.

### Device not connected/recognized

The following error message means that the device is not connected to your computer or the computer doesn't recognize the device.

```text
error: cannot access /dev/cu.SLAB_USBtoUART
```

There are a few reasons this can happen:
 
- Your device is not plugged into your computer. Make sure it's plugged in when you run the build commands. 
- You have a USB cable that is power only. Make sure you're using a data sync-capable USB cable.
- The computer does not recognize your device. To fix this problem, follow the instructions for your operating system below.

#### macOS/Linux

To test whether your computer recognizes your device, unplug the device and enter the following command.

```text
ls /dev/cu*
```

Then plug in the device and repeat the same command. If nothing new appears in the terminal output, the device isn't being recognized by your computer. Make sure you have the correct VCP driver installed. 

If it is recognized, you now have the device name and you need to edit the `UPLOAD_PORT` environment variable. Enter the following command, replacing `/dev/cu.SLAB_USBtoUART` with the name of the device on your system.

```text
export UPLOAD_PORT=/dev/cu.SLAB_USBtoUART
```

#### Windows

Check the list of USB devices in Device Manager. If your device shows up as an unknown device, make sure you have the correct VCP driver installed.

If your device shows up on a COM port other than COM3, you need to edit the `UPLOAD_PORT` environment variable. Enter the following command, replacing `COM3` with the appropriate device COM port for your system.

```text
set UPLOAD_PORT=COM3
```

### Incompatible baud rate

The following warning message is normal and is no cause for concern.

```text
warning: serialport_set_baudrate: baud rate 921600 may not work
```

However, sometimes the upload starts but does not complete. You can tell an upload is complete after the progress bar traced to the console goes to 100%. For example:

```text
........................................................... [ 16% ]
........................................................... [ 33% ]
........................................................... [ 49% ]
........................................................... [ 66% ]
........................................................... [ 82% ]
........................................................... [ 99% ]
..                                                         [ 100% ]
```

There are a few reasons the upload may fail partway through:

- You have a faulty USB cable.
- You have a USB cable that does not support higher baud rates.
- You're using a board that requires a lower baud rate than the default baud rate that the Moddable SDK uses.

To solve the last two problems above, you can change to a slower baud rate as follows: 

1. If you're working with an ESP32, open `moddable/tools/mcconfig/make.esp32.mk`; if an ESP8266, open `moddable/tools/mcconfig/make.esp.mk`. 

2. Find this line, which sets the upload speed to 921600:

    ```text 
    UPLOAD_SPEED ?= 921600
    ```

3. Set the speed to a smaller number. For example:

    ```text 
    UPLOAD_SPEED ?= 115200
    ```
 
### ESP32 is not in bootloader mode

If an ESP32 is not in bootloader mode, you cannot flash the device. Most development boards built with the ESP32 include circuitry that automatically puts them into bootloader mode when you try to reflash the board. Some do not, and sometimes the auto programming will fail. This is most common on Windows machines. 

When your ESP32 is not in bootloader mode, status messages stop being traced briefly when you attempt to flash the device, and after several seconds this error message is traced to the console:

```text
A fatal error occurred: Failed to connect to ESP32: Timed out waiting for packet header
```

To manually put your ESP32 into bootloader mode, follow these steps:

1. Unplug the device.
2. Hold down the BOOT button.
3. Plug the device into your computer.
4. Enter the `mcconfig` command.
5. Wait a few seconds and release the BOOT button.

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

	 ```text
	 ESP_SDK_RELEASE ?= esp8266-2.3.0
	 ```
	 
	 to:
	 
	 ```text
	 ESP_SDK_RELEASE ?= esp8266-2.4.0
	 ```

- Do a clean build of tools by deleting the contents of `${MODDABLE}/build/bin/esp/` and `${MODDABLE}/build/tmp/esp/` and building as above.
