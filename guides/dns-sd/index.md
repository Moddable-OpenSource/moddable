---
name: DNS Service Discovery
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-08-04
---

DNS Service Discovery (DNS-SD) lets devices advertise and discover services, such as HTTP servers or AirPlay speakers, on a local network. It works over Multicast DNS (mDNS), so no central server is required. The ECMA-419 embedded standard [DNS-SD](https://419.ecma-international.org/#dns-sd) API is available to Embedded JavaScript developers using the Moddable SDK, accessed through the host provider as `device.network.dnssd`.

A DNS-SD instance performs three kinds of operations: claiming a local name, advertising services, and discovering services.

## Using ECMA-419 DNS-SD

- [Claim Local Name](./claim.md)
- [Advertise Services](./advertise.md)
- [Discover Services](./discover.md)

## Creating a DNS-SD Instance

All DNS-SD operations are methods of an instance created from `device.network.dnssd`. If possible, share a single instance for all claim, advertise, and discover operations. Doing so is more efficient, though not a strict requirement. The guides assume a shared instance named `dnssd`, created as shown here, and so do not repeat this step in each example.

```js
const dnssd = new (device.network.dnssd.io)(device.network.dnssd);
```

Close the instance when DNS-SD services are no longer needed. Closing the instance releases all local name claims, ends advertising of services, and service discovery. Many projects never close the instance, keeping a DNS-SD instance for their entire lifetime.

```js
dnssd.close();
```

## Building with mcconfig

Include the DNS-SD manifest in your project's `manifest.json`. Including the manifest initializes the host provider `device.network.dnssd`:

	$(MODDABLE)/modules/embedded/network/dnssd/manifest.json


## Building with mcpack

Import the module in your JavaScript source code. `mcpack` automatically includes the manifest which initializes `device.network.dnssd`.

```js
import DNSSD from "embedded:network/dnssd";
```

## Learn More

- Standards
	- [DNS-SD Class Pattern](https://419.ecma-international.org/#dns-sd) in ECMA-419
	- [DNS-Based Service Discovery RFC](https://www.rfc-editor.org/info/rfc6763/)
	- [Multicast DNS RFC](https://www.rfc-editor.org/info/rfc6762/)
- Example
	- [DNS-SD example](../../examples/io/dnssd/main.js) in Moddable SDK
- Implementation
	- [DNS-SD](../../modules/embedded/network/dnssd/dnssd.js) in Moddable SDK
- TypeScript Declaration
	- [DNS-SD](../../typings/embedded_network/dnssd.d.ts) in Moddable SDK
