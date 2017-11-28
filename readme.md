# Moddable SDK
Copyright 2017 Moddable Tech, Inc.

Revised: November 27, 2017

This document is a guide to the Moddable SDK source code repository. 

## Modern software development for microcontrollers
The Moddable SDK is a combination of development tools and runtime software focused on creating software for microcontrollers.

Microcontrollers are highly constrained devices compared to modern computers and mobile devices. A typical microcontroller used with the Moddable SDK has about 45 KB of free memory, 1 MB of Flash ROM, and runs at 80 MHz. The Moddable SDK uses many different techniques, both at build time and at run time, to work efficiently on these devices.

The primary programming language for development is JavaScript. The XS JavaScript engine at the center of the Moddable SDK implements the [2017 JavaScript language standard](http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf) with better than 99% conformance. The constraints of the target microcontroller may limit the number of language features that can be used in combination by a single application.

The JavaScript language implemented in the Moddable SDK is the same language used in web pages and Node.js. The microcontroller that the scripts run on, however, is very different from a personal computer, server, or mobile device. These differences often require a different approach to using JavaScript. The APIs and objects in the Moddable SDK are quite different, being designed with the goal of minimizing memory use. Bring your existing experience with JavaScript, but be prepared to think about performance, code size, and memory use in a different ways.

As much as practical, the Moddable SDK is implemented in JavaScript. Portions of the Moddable SDK are implemented in C for performance or direct access to native APIs. There is no C++.

A significant part of building efficient software for microcontrollers occurs at build time. The Moddable SDK contains many tools and options for the build process. Take time to learn about these to get the best results.

## Getting started
The Moddable SDK is intended for use with micro-controller hardware. It includes simulators that run on macOS, Linux, and Windows. These are a great way to get started, and invaluable as development accelerators.

The [Getting Started](documentation/Moddable%20SDK%20–%20Getting%20Started.md) document provides step-by-step instructions for installing, configuring, and building the Moddable SDK for macOS, Linux, and Windows. It also includes instructions for building the example applications for the ESP8266 and ESP32 microcontrollers.

## Source tree
The Moddable SDK repository contains the following top level directories.

- **build** - Files required for specific microcontroller targets, the simulator, and make files for build tools in the `tools` directory.
- **documentation** - All the documentation for the Moddable SDK. Documentation is provided in markdown format.
- **examples** - Example applications for many of the capabilities of the Moddable SDK. Many example applications are under one page of source code to focus on demonstrating how to use a specific capability. Not every example is compatible with every device target.
- **license** - The license agreements for the software provided in the Moddable SDK. The Contributor License Agreements are here as well.
- **modules** - The software modules that make up the runtime of the Moddable SDK. These include networking, graphics, user interface, hardware access, cryptographic primitives, and device drivers. All modules have a JavaScript API. Many modules are implemented in part using C.
- **tools** - Tools to build applications using the Moddable SDK. These include command line tools for image format conversion, image compression, image rotation, font compression, processing localization strings, compiling resources, and building applications from JSON manifest files. In addition, xsbug, the XS source level debugger, is here.
- **xs** - The XS JavaScript engine including its compiler and linker, and the test262 execution shell.

The **documentation**, **examples**, and **modules** directories share a common structure to make it straightforward to locate information.

- **base** - fundamental runtime capabilities including time, timer, debug, instrumentation, and uuid
- **commodetto** - bitmap graphics library
- **crypt** - cryptographic primitives and TLS
- **data** - base64 and hex encoding
- **drivers** - device drivers for displays, touch inputs, sensors, and expanders
- **files** - storage capabilities including files, flash, preferences, resources, and zip
- **input** - on-screen keyboard input
- **network** - network socket and protocols built on socket including HTTP, WebSockets, DNS, SNTP, and telnet. Also, Wi-Fi API
- **pins** - Hardware protocols include digital (GPIO), analog, PWM, and I2C.
- **piu** - user interface framework

## Licensing
The Moddable SDK is provided under a combination of licenses that includes GPL 3.0, LGPL 3.0, Apache 2.0, and Creative Commons Attribution 4.0 Licenses. The license directory contains additional information on the licenses used and licensing options.
