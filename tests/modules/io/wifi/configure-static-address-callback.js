/*---
description: configure with static IP fires onChanged with "address" when connected
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();

let expectedAddress;

const wifi = new WiFi({
	onChanged(property) {
		try {
			if ("address" === property) {
				assert.sameValue(wifi.address, expectedAddress, "address should reflect the new static IP");
				wifi.close();
				$DONE();
				return;
			}
			if ("connection" !== property) return;
			if (wifi.connection >= 500) {
				const ip = wifi.address;
				const parts = ip.split(".").map(Number);
				parts[3] = ((parts[3] + 1) & 0xff) || 2;
				expectedAddress = parts.join(".");
				parts[3] = 1;
				const gateway = parts.join(".");
				wifi.configure({static: {address: expectedAddress, mask: "255.255.255.0", gateway}});
			}
		}
		catch (e) {
			$DONE(e);
		}
	}
});

wifi.connect({SSID: opts.ssid, password: opts.password});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
