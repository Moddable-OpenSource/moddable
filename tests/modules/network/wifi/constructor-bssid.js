/*---
description: 
flags: [async, module]
---*/

import WiFi from "wifi";
import Net from "net";
import Timer from "timer";

if (1 !== WiFi.mode) {
	WiFi.mode = 1;
	Timer.delay(3000);
}
WiFi.disconnect();
const options = await $NETWORK.wifi();

let target;
WiFi.scan(options, ap => {
	if (!ap) {
		if (!target)
			return $DONE("cannot find " + options.ssid);

		options.bssid = target.bssid;
		const monitor = new WiFi(options, (msg, value) => {
			try {
				if (WiFi.gotIP == msg) {
					let bssid = Net.get("BSSID").split(":").map(i => parseInt(i, 16));
					bssid = Uint8Array.from(bssid);
					let t = new Uint8Array(target.bssid);
					assert.sameValue(bssid.length, 6, "bad bssid length");
					assert.sameValue(t.length, 6, "bad bssid length");
					for (let i = 0; i < 6; i++)
						assert.sameValue(bssid[i], t[i], "unexpected bssid");
					$DONE();
				}
			}
			catch (e) {
				$DONE(e);
			}
		});
	}
	else if (ap.ssid === options.ssid)
		target = ap;
});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
