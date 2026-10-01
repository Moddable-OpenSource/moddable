---
name: Diagnostics
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

The TLS implementation provides two options to help diagnose problems with TLS connections. These options should never be used in production. In particular, disabling certificate validation defeats the security guarantees of a TLS connection.

---

The TLS implementation can log its progress through various steps of establishing a connection. This may provide insight into why the connection is failing. To enable logging, set `trace` to true in the `tls` options object.

```js
const http = new device.network.https.client.io({
	...device.network.https.client,
	socket: {
		...device.network.https.client.socket,
		tls: {
			...device.network.https.client.socket.tls,
			trace: true
		}
	},
	host: "example.com"
});
```

---

Disable certificate validation on the TLS connection by setting `verify` to `false` in the `tls` options object. Only use this capability for debugging, never in production.

```js
const http = new device.network.https.client.io({
	...device.network.https.client,
	socket: {
		...device.network.https.client.socket,
		tls: {
			...device.network.https.client.socket.tls,
			verify: false
		}
	},
	host: "example.com"
});
```
