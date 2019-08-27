## Moddable SDK - Documentation

Copyright 2019 Moddable Tech, Inc.<BR>
Revised: June 20, 2019

This folder contains documentation for the Moddable SDK.

### Getting Started Guides

The [Getting Started Guide](./Moddable%20SDK%20-%20Getting%20Started.md) provides step-by-step instructions for installing, configuring, and building the Moddable SDK for macOS, Linux, and Windows. It also includes instructions for building and running example applications on the ESP8266 and ESP32 microcontrollers.

Guides for specific devices supported by the Moddable SDK, including Moddable One and Moddable Two, are available in the [devices](./devices) directory.

### APIs for Moddable SDK Modules

The JavaScript APIs for the modules in the Moddable SDK are documented in the following folders:

- **base**: Fundamental runtime capabilities including time, timer, debug, instrumentation, and UUID
- **commodetto**: Bitmap graphics library
- **crypt**: Cryptographic primitives and TLS
- **data**: Base64 and hex encoding
- **files**: Storage capabilities including files, flash, preferences, resources, and zip
- **network**: Network socket and protocols built on socket including HTTP, WebSockets, DNS, SNTP, telnet, and TLS; also, Wi-Fi and BLE APIs.
- **pins**: Hardware protocols including digital (GPIO), analog, PWM, and I2C
- **piu**: User interface framework

### XS JavaScript Engine

The [xs](./xs) folder contains documentation related to the XS JavaScript engine.

### Developer Tools

The [tools](./tools) folder contains documentation for developer tools. This includes tools used directly by developers (such as our command line build tools and hardware simulator) and tools used indirectly by our build system.