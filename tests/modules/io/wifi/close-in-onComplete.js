/*---
description: closing the WiFi instance inside the onComplete callback is safe
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
wifi.scan({
	onFound() {},                 // intentionally no-op so we close only in onComplete
	onComplete() {
		try {
			wifi.close();
			$DONE();
		}
		catch (e) {
			$DONE(e);
		}
	}
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
