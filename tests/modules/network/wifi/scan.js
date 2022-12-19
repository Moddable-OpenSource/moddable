/*---
description: 
flags: [async, module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (1 !== WiFi.mode) {
	WiFi.mode = 1;
	Timer.delay(3000);
}

let count = 0;
const fresh = Symbol();
const authenticationValues = ["none", "wep", "wpa_psk", "wpa2_psk", "wpa_wpa2_psk"];

WiFi.scan({hidden: true}, ap => {
	if (!ap) {
		if (0 === count)
			return $DONE("no access point found");
		return $DONE();
	}
	
	try {
		assert.sameValue(ap[fresh], undefined, "not a fresh instance");

		count++;
		assert.sameValue(typeof ap.ssid, "string", "ssid");
		assert.sameValue(typeof ap.rssi, "number", "rssi");
		assert.sameValue(typeof ap.channel, "number", "channel");
		assert.sameValue(typeof ap.hidden, "boolean", "hidden");
		assert(ap.bssid instanceof ArrayBuffer, "bssid");
		if (ap.authentication) {
			assert.sameValue(typeof ap.authentication, "string", "authentication");
			assert(authenticationValues.includes(ap.authentication), "unknown authentication " + ap.authentication);
		}
		ap[fresh] = "stale";
	}
	catch (e) {
		$DONE(e);
	}
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
