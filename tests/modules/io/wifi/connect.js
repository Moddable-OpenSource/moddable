/*---
description: connect transitions through 300 (connecting) and reaches 500 (gotIP)
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();

let sawConnected = false;

const wifi = new WiFi({
	onChanged(property) {
		try {
			if ("connection" !== property) return;
			const state = wifi.connection;
			if (state >= 400 && state < 500)
				sawConnected = true;
			else if (state >= 500) {
				assert.sameValue(sawConnected, true, "should have observed connected without IP (>= 400, < 500)");
				wifi.close();
				$DONE();
			}
		}
		catch (e) {
			$DONE(e);
		}
	}
});

assert.sameValue(wifi.connection <= 200, true, "starts disconnected (<= 200)");
wifi.connect({SSID: opts.ssid, password: opts.password});
assert.sameValue(wifi.connection >= 300, true, "transitiions to connecting (>= 300)");

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
