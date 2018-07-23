# Moddable SDK – Getting Started
Copyright 2016-2017 Moddable Tech, Inc.

<!-- Last edit: 11/25/2017 BSF -->

Revised: November 25, 2017

This document provides an introduction to getting started building apps with the Moddable SDK. It describes how to configure the host build environments, install the required SDKs, drivers and development tools, build applications, and use xsbug, the JavaScript source code debugger.

## macOS

### Host environment setup

> The Moddable SDK requires macOS Sierra version 10.12 or newer and Xcode version 9 or newer.

1. Download and install [Xcode](https://developer.apple.com/xcode/). Launch Xcode to install additional command line components when prompted. 

2. Download the [Moddable repository] (https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```
	git clone https://github.com/Moddable-OpenSource/moddable
	```

3. Setup the `MODDABLE` environment variable to point at your local Moddable SDK repository directory and edit the `PATH` environment variable in your `~/.profile` to include the build directory:

	```
	export MODDABLE="/Users/<user>/Projects/moddable"
	export PATH="${MODDABLE}/build/bin/mac/release:$PATH"
	```
	
4. Build the Moddable command line tools, simulator, and debugger from the command line:

	```
	cd ${MODDABLE}/build/makefiles/mac
	make
	```
	
5. Launch the `xsbug` debugger from the command line:

	```
	open ${MODDABLE}/build/bin/mac/release/xsbug.app
	```

6. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p mac
	```
 
### ESP8266 (Moddable Zero) setup

1. Complete "Host environment setup" for macOS.

2. Create an `esp` directory in your home directory at `~/esp` for required third party SDKs and tools.
 
3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

4. Download the [esptool](https://github.com/igrr/esptool-ck/releases/download/0.4.12/esptool-0.4.12-osx.tar.gz). Untar the package and rename the directory `esptool`. Copy the `esptool` directory into the `~/esp` directory.

5. Download and untar the [Xtensa lx106 architecture GCC toolchain](http://www.moddable.tech/private/esp8266.toolchain.darwin.tgz). Copy the `toolchain` directory into the `~/esp` directory.

6. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `~/esp` directory.

7. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```
	cd ~/esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```
	
8. Connect the ESP8266 to your computer with a USB cable.

9. Verify the setup by building `helloworld` for the `esp` target:

	```
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p esp
	```

### ESP32 setup

1. Complete "Host environment setup" for macOS.

2. Create an `esp32` directory in your home directory at `~/esp32` for required third party SDKs and tools. 

3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

4. Download the [esptool](https://github.com/igrr/esptool-ck/releases/download/0.4.12/esptool-0.4.12-osx.tar.gz). Untar the package and rename the directory `esptool`. Copy the `esptool` directory into the `~/esp32` directory.

5. Download and untar the [ESP32 GCC toolchain](https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-73-ge28a011-5.2.0.tar.gz). Copy the extracted `xtensa-esp32-elf` directory into your `~/esp32` directory.

6. Clone the `ESP-IDF` GitHub repository into your `~/esp32` directory. Make sure to specify the `--recursive` option:

	```
	cd ~/esp32
	git clone --recursive https://github.com/espressif/esp-idf.git
	```
	
7. Update the `PATH` environment variable in your `~/.profile` to include the toolchain directory:

	```
	export PATH=$PATH:$HOME/esp32/xtensa-esp32-elf/bin
	```
		
8. Connect the ESP32 device to your macOS host with a USB cable.

9. Verify the setup by building `helloworld` for the `esp32` target:


	```
	cd ${MODDABLE}/examples/helloworld
	mcconfig -d -m -p esp32
	```
	
> Note that the first time you build an application for the ESP32 target, the toolchain may prompt you to enter configuration options. If this happens, accept the defaults.

## Windows

#### Host environment setup

> The Moddable SDK requires Windows 7 Pro SP1 or newer and Microsoft Visual Studio Community 2017 or newer.

1. Download [Microsoft Visual Studio 2017 Community Edition installer](https://www.visualstudio.com/downloads/). Launch the installer, choose the "Desktop development for C++" option and install. 

2. Download the [Moddable repository] (https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```
	git clone https://github.com/Moddable-OpenSource/moddable
	```

3. Setup the `MODDABLE` environment variable to point at your local Moddable SDK repository directory:

	```
	set MODDABLE=C:\Users\<user>\Projects\moddable
	```
	
4. Edit the system `PATH` environment variable to include the build directory:

	```
	%MODDABLE%\build\bin\win\release
	```
	
	> Environment variables should be set from the System Control Panel. The steps required vary depending on the Windows OS version.
	
4. Launch the "Developer Command Prompt for VS 2017" command line console. Build the Moddable command line tools, simulator, and debugger from the command line:

	```
	cd %MODDABLE%\build\makefiles\win
	build
	```
	
5. Launch the `xsbug` debugger from the command line:

	```
	xsbug
	```
	
6. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p win
	```
	
### ESP8266 (Moddable Zero) setup

1. Complete "Host environment setup" for Windows.

2. Create an `esp` directory in your home `%USERPROFILE%` directory, e.g. `C:\Users\<your-user-name>`.
 
3. Download and install the Silicon Labs [CP210x USB to UART VCP driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

4. Download the [esptool](https://github.com/igrr/esptool-ck/releases/download/0.4.12/esptool-0.4.12-win32.zip). Unzip the archive and copy the `esptool.exe` executable from the `esptool-0.4.12-win32` directory into the `esp` directory.

5. Download and unzip the [Cygwin toolchain support package](http://www.moddable.tech/private/cygwin.win32.zip). Copy the `cygwin` directory into the `esp` directory.
	
6. Download and unzip the [Xtensa lx106 architecture GCC toolchain](http://www.moddable.tech/private/esp8266.toolchain.win32.zip). Copy the `xtensa-lx106-elf` directory into the `esp` directory.

7. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `esp` directory.

8. Connect the ESP8266 to your computer with a USB cable.

9. Launch the Windows Device Manager, open the "Ports (COM & LPT)" section, and verify the "Silicon Labs CP210x USB to UART Bridge" is displayed. Note the COM port (e.g. COM3) for the next step.

	> The Device Manager interface may vary depending on the Windows OS version.
	
10. Set the `BASE_DIR` and `UPLOAD_PORT` environment variables to your `%USERPROFILE%` directory and device COM port:

	```
	set BASE_DIR=%USERPROFILE%
	set UPLOAD_PORT=COM3
	```

11. Edit the system `PATH` environment variable to include the `cygwin\bin` directory:

	```
	%BASE_DIR%\esp\cygwin\bin
	```

12. Launch the "Developer Command Prompt for VS 2017" command line console. Verify the setup by building `helloworld` for the `esp` target:

	```
	cd %MODDABLE%\examples\helloworld
	mcconfig -d -m -p esp
	```

## Linux

### Host environment setup

> The Moddable SDK has been tested on the Ubuntu 16.04 LTS (64-bit) and Raspberry Pi Desktop (32-bit) operating systems. These setup instructions assume that a GCC toolchain has already been installed.

1. Install the development version of the GTK+ 3 library:

	```
	sudo apt-get install libgtk-3-dev
	```
	
2. Download the [Moddable repository] (https://github.com/Moddable-OpenSource/moddable), or use the `git` command line tool as follows:

	```
	git clone https://github.com/Moddable-OpenSource/moddable
	```

3. Setup the `MODDABLE` environment variable in your `~/.bashrc` file to point at your local Moddable SDK repository directory:

	```
	MODDABLE=~/Projects/moddable
	export MODDABLE
	```
	
4. Build the Moddable command line tools, simulator, and debugger from the command line:

	```
	cd $MODDABLE/build/makefiles/lin
	make
	```
	
5. Update the `PATH` environment variable in your `~/.bashrc` to include the tools directory:

	```
	export PATH=$PATH:$MODDABLE/build/bin/lin/release
	```

6. Install the Screen Test desktop simulator and xsbug debugger applications:

	```
	cd $MODDABLE/build/makefiles/lin
	make install
	```

	When prompted, enter your `sudo` password to copy the application's desktop, executable and icon files into the standard `/usr/share/applications`, `/usr/bin`, and `/usr/share/icon/hicolor` directories.
	
7. Launch the xsbug debugger:

	```
	xsbug
	```
	
8. Verify the host environment setup by building the starter `helloworld` application for the desktop simulator target:

	```
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p lin
	```
	
### ESP8266 (Moddable Zero) setup

1. Complete "Host environment setup" for Linux.

2. Create an `esp` directory in your home directory at `~/esp` for required third party SDKs and tools.
 
3. Download the [esptool](https://github.com/igrr/esptool-ck/releases) compatible with your Linux host. Untar the package and rename the directory `esptool`. Copy the `esptool` directory into the `~/esp` directory.

4. Download and untar the [Xtensa lx106 architecture GCC toolchain](http://www.moddable.tech/private/esp8266.toolchain.linux.tgz). Copy the `toolchain` directory into the `~/esp` directory.

5. Download the [ESP8266 core for Arduino repository](https://github.com/esp8266/Arduino/releases/download/2.3.0/esp8266-2.3.0.zip). Copy the extracted `esp8266-2.3.0` folder into your `~/esp` directory.

6. Clone the [ESP8266 SDK based on FreeRTOS](https://github.com/espressif/ESP8266_RTOS_SDK) repository into the `~/esp` directory:

	```
	cd ~/esp
	git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
	```
	
7. Connect the ESP8266 to your computer with a USB cable.

8. Verify the setup by building `helloworld` for the `esp` target:

	```
	cd $MODDABLE/examples/helloworld
	mcconfig -d -m -p esp
	```

## Debugging applications
The `xsbug` JavaScript source level debugger is built as part of the Moddable SDK build described above. `xsbug` is a full featured debugger that supports debugging modules and applications for [XS platforms](https://github.com/Moddable-OpenSource/moddable/blob/master/documentation/xs/XS%20Platforms.md). The `xsbug` debugger is automatically launched when deploying debug builds and connects to devices via USB or over Wi-Fi. Similar to other debuggers, `xsbug` supports setting breakpoints, browsing source code, the call stack and variables. The `xsbug` debugger additionally provides real-time instrumentation to track memory usage and profile application and resource consumption.

> For additional details on `xsbug` please refer to the [notes.md](https://github.com/Kinoma/kinomajs/blob/master/xs6/xsbug6/doc/notes.md) document in the [KinomaJS](https://github.com/Kinoma/kinomajs) open source repository.

