---
name: Transport Layer Security (TLS)
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

TLS is available to secure network connections. Most projects do not create a TLS instance directly. Instead, they do so indirectly by using secure network protocols like HTTPS, WSS, and MQTTS.

Configuration is the primary challenge when using TLS. This can be more difficult than using TLS on a computer because of the limited capacity of embedded devices and the widespread use of private certificates in IoT products.

## Using TLS

- [Include Public Certificates](./certificates-public.md)
- [Include Private Certificates](./certificates-private.md)
- [Diagnostics](./diagnostics.md)
- [DER and PEM Certificates](./pemdir.md)

## Learn More

- Standards
	- [TLS Client Socket](https://419.ecma-international.org/#io-classes-tls-client-socket) in ECMA-419
	- [TLS 1.2 RFC](https://www.rfc-editor.org/info/rfc5246/)
- Implementations
	- [TLS Socket](../../modules/io/tls/client/tlssocket.js) in Moddable SDK
	- [TLS Session](../../modules/crypt/ssl/session_419.js) in Moddable SDK
- TypeScript Declarations
	- [TLS Socket](../../typings/embedded_io/socket/tcp/tls.d.ts) in Moddable SDK
