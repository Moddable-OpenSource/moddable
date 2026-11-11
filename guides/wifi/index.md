---
name: Wi-Fi
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

Wi-Fi provides the network connection for the majority of Embedded JavaScript projects. The ECMA-419 embedded standard [Wi-Fi Network Interface](https://419.ecma-international.org/#network-interface-class-pattern-wi-fi-network-interface) is available scans for access points, connects to a network, and monitors the connection.

The Moddable SDK provides Wi-Fi implementations for ESP32, ESP8266, Raspberry Pi Pico, and Zephyr. There is a [simulator implementation](../../modules/embedded/network/interface/wifi/sim/readme.md) to exercise Wi-Fi APIs without hardware.

## Using ECMA-419 Wi-Fi Network Interface

- [Scan for Access Points](./wifi-scan.md)
- [Scan Continuously for Access Points](./wifi-continuous-scan.md)
- [Connect](./wifi-connect.md)
- [Reconnect Automatically](./wifi-reconnect.md)
- [Disconnect](./wifi-disconnect.md)
- [Get Connection Information](./wifi-connection-info.md)
- [Use Static IP Address](./wifi-using-static-ip-address.md)

## Building with mcconfig

The Wi-Fi Network Interface is part of the standard networking manifest, `manifest_net.json`, so most networked projects already include it. To add it directly, include its manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/embedded/network/interface/wifi

Then, import the module in your JavaScript source code:

```js
import WiFi from "embedded:network/interface/wifi";
```

## Building with mcpack

Import the module in your JavaScript source code. `mcpack` automatically includes the manifest.

```js
import WiFi from "embedded:network/interface/wifi";
```

## Learn More

- Standard
	- [Wi-Fi Network Interface](https://419.ecma-international.org/#network-interface-class-pattern-wi-fi-network-interface) in ECMA-419
- Implementation
	- [Wi-Fi Network Interface](../../modules/embedded/network/interface/wifi/) in Moddable SDK
- TypeScript Declaration
	- [Wi-Fi Network Interface](../../typings/embedded_network/interface/wifi.d.ts) in Moddable SDK
