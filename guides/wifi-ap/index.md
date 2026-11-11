---
name: Wi-Fi Access Point
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

The Wi-Fi Access Point APIs create a Wi-Fi base station running on an embedded device. Other devices can connect to the access point to access its services. Wi-Fi Access Points on embedded devices are commonly used for local device provisioning flows. The Moddable SDK provides the [Captive Portal module](../../examples/io/wifiaccesspoint/captiveportal/) for provisioning, building on a Wi-Fi Access Point. The Wi-Fi Access Point APIs are an [ECMA-419 Network Interface](https://419.ecma-international.org/#network-interface-class-pattern) with extensions that are expected to be defined as part of ECMA-419 5th Edition.

Most microcontrollers allow a device to be a Wi-Fi Access Point and at the same time also be connected to another Wi-Fi Access Point. Wi-Fi performance is often reduced when both are in use.

The Moddable SDK provides Wi-Fi Access Point support for ESP32, ESP8266, Raspberry Pi Pico, and Zephyr. There is a [simulator implementation](../../modules/embedded/network/interface/wifi/accesspoint/sim/readme.md) to exercise Wi-Fi Access Point APIs without hardware.

## Using ECMA-419 Wi-Fi Access Point Network Interface

- [Create Access Point](./create.md)
- [Close Access Point](./close.md)
- [Get Information](./info.md)
- [Manage Stations](./stations.md)
- [DNS Redirect](./dns.md)
- [Provide HTTP Server](./httpserver.md)

## Building with mcconfig

Include the Wi-Fi Access Point Network Interface manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/embedded/network/interface/wifi/accesspoint

Then, import the module in your JavaScript source code:

```js
import WiFiAccessPoint from "embedded:network/interface/wifi/accesspoint";
```

## Building with mcpack

Import the module in your JavaScript source code. `mcpack` automatically includes the manifest.

```js
import WiFiAccessPoint from "embedded:network/interface/wifi/accesspoint";
```

## Learn More

- Standard
	- [Network Interface Class Pattern](https://419.ecma-international.org/#network-interface-class-pattern) in ECMA-419
- Implementation
	- [Wi-Fi Access Point Network Interface](../../modules/embedded/network/interface/wifi/accesspoint/) in Moddable SDK
- Examples
	- [Wi-Fi Access Point](../../examples/io/wifiaccesspoint/basic/main.js) in Moddable SDK
	- [Captive Portal](../../examples/io/wifiaccesspoint/captiveportal/captiveportal.js) in Moddable SDK
- TypeScript Declaration
	- [Wi-Fi Access Point Network Interface](../../typings/embedded_network/interface/wifi/accesspoint.d.ts) in Moddable SDK
