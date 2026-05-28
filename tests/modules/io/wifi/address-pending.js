/*---
description: address is undefined in any state before IP assignment (< 500)
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
			const state = wifi.connection;
			if (state < 500) {
				// Spec: address is undefined "if the address has not yet been assigned".
				assert.sameValue(wifi.address, undefined, "address should be undefined at state " + state);
				return;
			}
			// state >= 500: address must now be assigned.
			assert.sameValue(typeof wifi.address, "string", "address should be a string once state >= 500");
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
