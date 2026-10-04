---
name: Advertise Services
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

These examples use a shared `dnssd` instance, as shown in [Creating a DNS-SD Instance](./index.md#creating-a-dns-sd-instance).

Use `advertise()` to advertise a single DNS-SD service.

Services are associated with a local name. Before advertising a service, you should [claim the local name](./claim.md) for your device.

This example advertises an HTTP server available on port 8080 of `example.local` with the name "419 Web Server". Note that the "name" is a human-readable name that is independent of the local name.

The actual service, here an HTTP server, is implemented separately from DNS-SD. DNS-SD is only to indicate its availability to other devices on the local network.

If your device has multiple services, call `advertise()` once for each service.

```js
dnssd.advertise({
	serviceType: "_http._tcp",
	name: "419 Web Server",
	host: "example",
	port: 8080
});
```

---

To stop advertising the service and announce that it is no longer available, close the returned advertising instance.

```js
const httpServerAd = dnssd.advertise({
	serviceType: "_http._tcp",
	name: "419 Web Server",
	host: "example",
	port: 8080
});

// some time later
httpServerAd.close();
```

---

You can include additional information about your service using the DNS `txt` record. This is a collection of key-value pairs in a [`Map`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map).

The `txt` record can contain text and binary values. Because the entire `txt` record must fit in a single DNS packet, the total data size must be small.

```js
dnssd.advertise({
	serviceType: "_http._tcp",
	name: "419 Web Server",
	host: "example",
	port: 8080,
	txt: new Map([
		["home", "/index.html"],
		["help", "/docs/help.html"]
	]),
});
```

---

Call `updateTXT()` to update the `txt` record to notify local devices of changes to the device. For example, a Wi-Fi speaker can use this to indicate the song being played and a printer can use it to indicate that it is currently busy printing.

You pass `updateTXT()` the entire `txt` record, not just the changed fields.

```js
let pagesServed = 0;

const httpServerAd = dnssd.advertise({
	serviceType: "_http._tcp",
	name: "419 Web Server",
	host: "example",
	port: 8080,
	txt: new Map([
		["home", "/index.html"],
		["pagesServed", "0"]
	])
});

// each time a page is served
pagesServed += 1;
httpServerAd.updateTXT(new Map([
	["home", "/index.html"],
	["pagesServed", pagesServed.toString()]
]));
```
