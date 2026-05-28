/*---
description: when connected, SSID/BSSID/RSSI/channel/address return sensible values
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
			if (wifi.connection < 500) return;

			assert.sameValue(typeof wifi.SSID, "string", "SSID is a string");
			assert.sameValue(wifi.SSID, opts.ssid, "SSID matches connected network");

			assert.sameValue(typeof wifi.BSSID, "string", "BSSID is a string");
			assert(/^[0-9a-f]{2}(:[0-9a-f]{2}){5}$/.test(wifi.BSSID), "BSSID format: " + wifi.BSSID);

			assert.sameValue(typeof wifi.RSSI, "number", "RSSI is a number");

			assert.sameValue(typeof wifi.channel, "number", "channel is a number");
			assert.sameValue(wifi.channel > 0, true, "channel is positive");

			assert.sameValue(typeof wifi.address, "string", "address is a string");
			// Spec: IPv4 as x.x.x.x (0-255 decimal) or IPv6 as 8 colon-separated lowercase 4-hex groups.
			const ipv4 = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;
			const ipv6 = /^([0-9a-f]{4}:){7}[0-9a-f]{4}$/;
			const m = wifi.address.match(ipv4);
			const valid = m ? m.slice(1).every(p => +p <= 255) : ipv6.test(wifi.address);
			assert.sameValue(valid, true, "address format: " + wifi.address);

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
