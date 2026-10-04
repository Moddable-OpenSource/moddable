---
name: Resolve Multiple Names
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

A single DNS resolver may be used to resolve several DNS names by calling `resolve()` multiple times.

The example resolves different kinds of host names in parallel. Results may be arrive in a different order from requests. The resolver is left open after resolution completes to be available to resolve additional names.

```js
import Resolver from "embedded:network/dns/resolver/udp";
import UDP from "embedded:io/socket/udp";

const dns = new Resolver({
	socket: { io: UDP },
	servers: ["8.8.8.8", "1.1.1.1"]
});

["moddable.com", "yahoo.com", "Desk-Lamp.local",
	"dfasdf.net",  "dfasdf.local", "localhost",
	"192.168.4.31"].forEach(host => {
		dns.resolve({
			host,
			onResolved(host, address) {
				trace(`Resolved ${host} to ${address}\n`);
			},
			onError(host) {
				trace(`Unable to resolve ${host}\n`);
			}
		});
	});
```
