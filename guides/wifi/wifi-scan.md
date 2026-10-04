---
name: Scan for Access Points
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Scanning for access points takes several seconds. The results of the scan are deliverered to `onFound()` and `onComplete()` is called when the scan is complete. A scan cannot be canceled.

```js
import WiFi from "embedded:network/interface/wifi";

const wifi = new WiFi({});
wifi.scan({
	onFound(ap) {
		trace(`Found ${ap.SSID} on channel ${ap.channel}\n`);
		trace(` "${ap.security}" security. RSSI ${ap.RSSI}\n`);
	},
	onComplete() {
		trace(`Wi-Fi scan complete\n`);
	}
});
```

---

Mesh networks deploy multiple access points with the same name to increase coverage. This can be confusing in user interface. You can remove duplicates using a [`Map`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map).

When two access points have the same name, this example chooses the access point with the strongest signal strength.

```js
import WiFi from "embedded:network/interface/wifi";

const wifi = new WiFi({});
const scan = new Map();

wifi.scan({
	onFound(ap) {
		const prev = scan.get(ap.SSID);
		if (!prev || prev.RSSI < ap.RSSI)
			scan.set(ap.SSID, ap);
	},
	onComplete() {
		scan.forEach(ap => {
			trace(`Found ${ap.SSID} on channel ${ap.channel}\n`);
			trace(` "${ap.security}" security. RSSI ${ap.RSSI}\n`);
		});
	}
});
```
