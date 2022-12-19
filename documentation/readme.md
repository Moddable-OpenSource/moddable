# Moddable SDK – Documentation Overview
Copyright 2019-2022 Moddable Tech, Inc.<BR>
Revised: March 18, 2020

This directory contains documentation for the Moddable SDK.

### Getting Started

The [Getting Started Guide](./Moddable%20SDK%20-%20Getting%20Started.md) provides step-by-step instructions for installing, configuring, and building the Moddable SDK for macOS, Linux, and Windows. It also provides instructions for keeping the Moddable SDK tools and build environment up to date over time.

Guides for working with specific microcontrollers supported by the Moddable SDK are in the  [devices](./devices) directory. These include links to additional detail for specific development boards including Moddable One, Moddable Two, and Moddable Three. 

- [ESP32](./devices/esp32.md) by Espressif
- [ESP8266](./devices/esp8266.md) by Espressif
- [Gecko](./devices/gecko/GeckoBuild.md) by Silicon Labs
- [QCA4020](./devices/qca4020/README.md) by Qualcomm
- [Pico](./devices/pico.md) by Raspberry Pi

### API Documentation for Modules

The JavaScript APIs for the modules in the Moddable SDK are documented in the following files:

- [**Base**](./base/base.md): Fundamental runtime capabilities including time, timer, debug, instrumentation, and UUID
  - [**Setup**](./base/setup.md): Using `setup` modules to configure a host before other modules execute 
  - [**Worker**](./base/worker.md): Using Web Workers and Shared Workers 
- [**Commodetto**](./commodetto/commodetto.md): Bitmap graphics library including parsing and rendering of BMP, JPEG, and PNG images, and BMFont files; classes for operating on bitmaps, and pixel format conversion
  - [**Poco**](./commodetto/poco.md): Examples and reference for using the JavaScript and C APIs of the Poco renderer  
- [**Crypt**](./crypt/crypt.md): Cryptographic primitives
- [**Data**](./data/data.md): Base64 and hex encoding and decoding
 - [**Files**](./files/files.md): Storage capabilities including files, flash, preferences, resources, and ZIP
- [**Network**](./network/network.md): Network socket and protocols built on socket including HTTP, WebSockets, MQTT, mDMS, DNS, SNTP, telnet, and ping; also, Wi-Fi  APIs
  - [**Secure socket**](./network/securesocket.md): Using the TLS /SSL and managing certificates
  - [**BLE**](./network/ble/ble.md): Creating Bluetooth LE clients and servers, including a guide to examples
- [**Pins**](./pins/pins.md): Hardware protocols including digital (GPIO), analog, PWM, I2C, SMBus, and servo
  - [**Audio out**](./pins/audioout.md): Playing audio and adding audio data to projects
- [**Piu**](./piu/piu.md): User interface framework
  - [**Die cut**](./piu/die-cut.md): Using complex clipping shapes with Piu
  - [**Expanding keyboard**](./piu/expanding-keyboard.md): Adding an animated expanding keyboard to Piu projects
  - [**Keyboard**](./piu/keyboard.md): Adding a touch keyboard to Piu projects
  - [**Localization**](./piu/localization.md): Using JSON data to efficiently localize Piu applications

### XS JavaScript Engine

The [xs](./xs) directory contains documentation about the XS JavaScript engine.

- [**ROM Colors**](./xs/ROM%20Colors.md): Optimization applied by the XS linker to improve look-up speed of properties of objects stored in ROM
- [**XS Conformance**](./xs/XS%20Conformance.md): Detailed test results for XS from the test262 language test suite
- [**XS Differences**](./xs/XS%20Differences.md): Discussion of design considerations for implementing JavaScript for a resource constrained target
- [**XS Platforms**](./xs/XS%20Platforms.md): Porting XS to a new host
- [**XS in C**](./xs/XS%20in%20C.md): API to bridge between C and JavaScript code
- [**XS linker warnings**](./xs/XS%20linker%20warnings.md): Discussion of warnings given by the XS linker when preparing code to be stored in ROM
- [**Handle**](./xs/handle.md): How native implementations of JavaScript classes can reduce memory fragmentation
- [**Mods**](./xs/mods.md): Working with archives of precompiled JavaScript modules (mods)
- [**Preload**](./xs/preload.md): Reducing memory use and start-up time of JavaScript modules
- [**xsbug**](./xs/xsbug.md): Using xsbug, the source-level debugger for XS
- [**xst**](./xs/xst.md): Using `xst`, the command line tool for running test262 conformance tests

### Developer Tools

The [tools](./tools) directory contains documentation for developer tools.

- [**Tools**](./tools/tools.md): Command line build tools including `mcconfig`, `mcrez`, `png2bmp`, `xsc`, `xsl`, and desktop simulator
- [**Manifest**](./tools/manifest.md): Explanation of the JSON manifest files used to configure the build of projects
- [**Defines**](./tools/defines.md): Using the project manifest to configure native code options
