/*---
description: closing the WiFi instance inside the onFound callback is safe
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

let done = false;

const wifi = new WiFi({});
wifi.scan({
	onFound() {
		try {
			if (done) return;
			done = true;
			wifi.close();
			$DONE();
		}
		catch (e) {
			$DONE(e);
		}
	},
	onComplete() {
		try {
			if (done) return;       // empty-scan fallback
			done = true;
			wifi.close();
			$DONE();
		}
		catch (e) {
			$DONE(e);
		}
	}
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
