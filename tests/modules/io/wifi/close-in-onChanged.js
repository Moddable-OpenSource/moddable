/*---
description: closing the WiFi instance inside the onChanged callback is safe
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
