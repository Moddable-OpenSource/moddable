/*---
description: connect coerces SSID and password via Symbol.toPrimitive
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();

let done = false;

const wifi = new WiFi({
	onChanged(property) {
		try {
			if (done) return;
			if ("connection" !== property) return;
			if (wifi.connection >= 500) {
				done = true;
				wifi.close();
				$DONE();
			}
		}
		catch (e) {
			$DONE(e);
		}
	}
});

wifi.connect({
	SSID: {
		[Symbol.toPrimitive]() { return opts.ssid; }
	},
	password: {
		[Symbol.toPrimitive]() { return opts.password; }
	}
});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
