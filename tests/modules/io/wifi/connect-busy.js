/*---
description: a second connect call while already connecting throws
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();

const wifi = new WiFi({
	onChanged(property) {
		try {
			if ("connection" !== property) return;
			if (wifi.connection >= 500) {
				wifi.close();
				$DONE();
			}
		}
		catch (e) {
			$DONE(e);
		}
	}
});

wifi.connect({SSID: opts.ssid, password: opts.password});
assert.throws(Error, () => wifi.connect({SSID: opts.ssid, password: opts.password}), "second connect should throw");

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
