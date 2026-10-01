---
name: Include Private Certificates
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

A certificate can be passed directly to a network constructor as part of the options object. The easiest way to do this is to append the certificate to the platform default secure connection settings for the protocol you are using. For example, when making secure HTTP requests, you use `device.network.https.client`. Its `socket` property contains a `tls` property with the default TLS configuration.

This example passes one private certificate to be used by this HTTPS session. Certificates are always in DER format and stored in a [Byte Buffer](https://419.ecma-international.org/#byte-buffer) (see [DER and PEM Certificates](./pemdir.md)).

You can also pass an array of certificates for the `ca` property.

```js
const myPrivateCert = loadCertificate();
const http = new device.network.https.client.io({
	...device.network.https.client,
	socket: {
		...device.network.https.client.socket,
		tls: {
			...device.network.https.client.socket.tls,
			ca: myPrivateCert
		}
	},
	host: "example.com"
});
```

---

 Use the same technique to pass a certificate to a secure WebSocket connection.

```js
const myPrivateCert = loadCertificate();
const wsc = new device.network.wss.io({
	...device.network.wss,
	socket: {
		...device.network.wss.socket,
		tls: {
			...device.network.wss.socket.tls,
			ca: myPrivateCert,
		}
	},
	host: "example.com",
	path: "/ws",
});
```

---

Use the same technique to pass a certificate to a secure MQTT connection.

```js
const myPrivateCert = loadCertificate();
new device.network.mqtts.io({
	...device.network.mqtts,
	socket: {
		...device.network.mqtts.socket,
		tls: {
			...device.network.mqtts.socket.tls,
			ca: myPrivateCert,
		}
	},
	host: "test.mosquitto.org"
});
```

---

You can also pass a client certificate and client key.

If there are multiple client certificates and client keys, pass them in arrays for `clientCertificate` and `clientKey`.

This works in the same way for secure MQTT and WebSocket connections as well.

```js
const {privateClientCert, privateClientKey} = loadCertificateAndKey();
const http = new device.network.https.client.io({
	...device.network.https.client,
	socket: {
		...device.network.https.client.socket,
		tls: {
			...device.network.https.client.socket.tls,
			clientCertificate: privateClientCert,
			clientKey: privateClientKey
		}
	},
	host: "example.com"
});
```
