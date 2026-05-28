/*---
description: disconnect from gotIP returns connection to 200
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
				wifi.disconnect();
				assert.sameValue(wifi.connection <= 200, true, "disconnect should drop state to <= 200");
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

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
