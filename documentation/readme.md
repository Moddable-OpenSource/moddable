## Moddable SDK - Documentation

Copyright 2019-2020 Moddable Tech, Inc.<BR>
Revised: December 4, 2020

This directory contains documentation for the Moddable SDK.

### Getting Started

The [Getting Started Guide](./Moddable%20SDK%20-%20Getting%20Started.md) provides step-by-step instructions for installing, configuring, and building the Moddable SDK for macOS, Linux, and Windows. It also provides instructions for keeping the Moddable SDK tools and build environment up to date over time.

Guides for working with specific microcontrollers supported by the Moddable SDK are in the  [devices](./devices) directory. These include links to additional detail for specific development boards including Moddable One, Moddable Two and Moddable Three. 

- [ESP32](./devices/esp32.md) by Espressif
- [ESP8266](./devices/esp8266.md) by Espressif
- [Gecko](./devices/gecko/GeckoBuild.md) by Silicon Labs
- [QCA4020](./devices/qca4020/README.md) by Qualcomm

### API Documentation for Modules in  the Moddable SDK

The JavaScript APIs for the modules in the Moddable SDK are documented in the following directories:

- [**base**](./base/base.md): Fundamental runtime capabilities including time, timer, debug, instrumentation, and UUID
 - [**setup**](./base/setup.md): Using `setup` modules to configure a host before other modules execute 
 - [**worker**](./base/worker.md): Using the implementation of Web Workers and Shared Workers 
- [**commodetto**](./commodetto/commodetto.md): The bitmap graphics library including parsing BMP, JPEG, PNG, and BMFont files, classes for operating on bitmaps, and pixel format conversion
 - [**poco**](./commodetto/poco.md): Examples and reference for using the JavaScript and C APIs of the Poco renderer  
- [**crypt**](./crypt/crypt.md): Cryptographic primitives
- [**data**](./data/data.md): Base64 and hex encoding
- [**files**](./files/files.md): Storage capabilities including files, flash, preferences, resources, and ZIP
- [**network**](./network/network.md): Network socket and protocols built on socket including HTTP, WebSockets, MQTT, mDMS, DNS, SNTP, telnet, and ping; also, Wi-Fi  APIs.
 - [**secure socket**](./network/securesocket.md): Using the TLS /SSL implementation and managing certificates
 - [**BLE**](./network/ble/ble.md): Creating BLE clients and servers, including a guide to examples
- [**pins**](./pins/pins.md): Hardware protocols including digital (GPIO), analog, PWM, I2C, SMBus, and servo.
 - [**audio out**](./pins/audioout.md): Playing audio and adding audio data to projects
- [**piu**](./piu/piu.md): User interface framework
 - [**die cut**](./piu/die-cut.md): Using complex clipping shapes with Piu
 - [**expanding keyboard**](./piu/expanding-keyboard.md): Adding an animated expanding keyboard to Piu projects
 - [**keyboard**](./piu/keyboard.md): Adding a touch keyboard to Piu projects
 - [**localization**](./piu/localization.md): Using JSON data to efficiently localize Piu applications

### XS JavaScript Engine

The [xs](./xs) directory contains documentation about the XS JavaScript engine.

- [**ROM Colors**](./xs/ROM Colors.md): Optimization applied by the XS linker to improve look-up speed of object properties stored in ROM
- [**XS Conformance**](./xs/XS Conformance.md): Detailed results of XS from the test262 language test suite.
- [**XS Differences**](./xs/XS Differences.md): Discussion of design considerations for implementing JavaScript on a microcontroller
- [**XS Platforms**](./xs/XS Platforms.md): Porting XS to a new host
- [**XS in C**](./xs/XS in C.md): The API for bridging between C and JavaScript code
- [**XS linker warnings**](./xs/XS linker warnings.md): Explanation of warnings given by the XS linker when preparing code to be stored in ROM
- [**Handle**](./xs/handle.md): How native implementations of JavaScript classes can reduce memory fragmentation
- [**Mods**](./xs/mods.md): Working with mods, archives of precompiled JavaScript modules
- [**Preload**](./xs/preload.md): Reducing memory use and start-up time of JavaScript modules
- [**xsbug**](./xs/xsbug.md): Using xsbug, the source-level debugger for XS
- [**xst**](./xs/xst.md): Using xst, the command line tool for running test262 conformance tests

### Developer Tools

The [tools](./tools) directory contains documentation for developer tools.

- [**Tools**](./tools/tools.md): Command line build tools including mcconfig, mcrez, png2bmp, xsc, xsl, and desktop simulator
- [**Manifest**](./tools/manifest.md): Explanation of the JSON manifest files used to configure the build of projects
- [**Defines**](./tools/defines.md): Using the project manifest to configure native code options
