/*---
description: a normal DHCP connect fires onChanged("address") after connection reaches 500
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
			if ("address" !== property) return;
			assert.sameValue(wifi.connection >= 500, true, "address should fire only after connection >= 500");
			done = true;
			wifi.close();
			$DONE();
		}
		catch (e) {
			$DONE(e);
		}
	}
});

wifi.connect({SSID: opts.ssid, password: opts.password});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
