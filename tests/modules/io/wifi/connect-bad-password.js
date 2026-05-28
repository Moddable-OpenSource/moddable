/*---
description: connect with a wrong password must never reach gotIP (500)
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();
const badPassword = ("BAD-" + (opts.password || "x")).slice(0, 63);

let done = false;

const wifi = new WiFi({
	onChanged(property) {
		try {
			if (done) return;
			if ("connection" !== property) return;
			const state = wifi.connection;
			if (state >= 500) {
				done = true;
				$DONE("should not reach gotIP with bad password");
				return;
			}
			if (state <= 200) {
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

wifi.connect({SSID: opts.ssid, password: badPassword});

$TESTMC.timeout($TESTMC.wifiInvalidConnectionTimeout);
