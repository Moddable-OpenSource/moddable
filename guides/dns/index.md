---
name: DNS
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

Domain Name System (DNS) resolution converts host names, such as `moddable.com`, into IP addresses. The low-level ECMA-419 embedded standard [DNS Resolver](https://419.ecma-international.org/#dns-resolver) is available to Embedded JavaScript developers using the Moddable SDK.

The ECMA-419 DNS Resolver is used by networking APIs, including `fetch()`, `WebSocket`, and MQTT. You can also use it directly, for example when implementing your own network protocols. The default Moddable SDK resolver operates over [UDP](https://419.ecma-international.org/#dns-resolver-dns-over-udp).

## Using ECMA-419 DNS Resolver

- [Resolve Name](./dns-resolving-one.md)
- [Resolve Multiple Names](./dns-resolving-multiple.md)

## Building with mcconfig

Include the DNS Resolver manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/io/dns/client

Then, import the module in your JavaScript source code:

```js
import Resolver from "embedded:network/dns/resolver/udp";
```

## Building with mcpack

Import the module in your JavaScript source code. `mcpack` automatically includes the manifest.

```js
import Resolver from "embedded:network/dns/resolver/udp";
```

## Learn More

- Standards
	- [DNS Resolver](https://419.ecma-international.org/#dns-resolver) in ECMA-419
	- [DNS protocol specification RFC](https://www.rfc-editor.org/info/rfc1035/)
- Example
	- [DNS Resolver example](../../examples/io/udp/dns/main.js) in Moddable SDK
- Implementation
	- [DNS Resolver](../../modules/io/dns/client/dns.js) in Moddable SDK
- TypeScript Declaration
	- [DNS Resolver](../../typings/embedded_network/dns/resolver/udp.d.ts) in Moddable SDK
