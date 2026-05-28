/*---
description: scan delivers access points via onFound and signals onComplete
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
const seen = new Set();

wifi.scan({
	onFound(ap) {
		try {
			assert.sameValue(typeof ap.SSID, "string", "SSID");
			assert.sameValue(typeof ap.BSSID, "string", "BSSID");
			assert(/^[0-9a-f]{2}(:[0-9a-f]{2}){5}$/.test(ap.BSSID), "BSSID format: " + ap.BSSID);
			assert.sameValue(typeof ap.RSSI, "number", "RSSI");
			assert.sameValue(typeof ap.channel, "number", "channel");
			if (undefined !== ap.security)
				assert.sameValue(typeof ap.security, "string", "security is a string");
			seen.add(ap.BSSID);
		}
		catch (e) {
			$DONE(e);
		}
	},
	onComplete() {
		try {
			assert(seen.size > 0, "at least one access point found");
			wifi.close();
			$DONE();
		}
		catch (e) {
			$DONE(e);
		}
	}
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
