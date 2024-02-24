/*---
description: 
flags: [asymc, module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (WiFi.Mode.station !== WiFi.mode) {
	WiFi.mode = WiFi.Mode.station;
	Timer.delay(3000);
}
WiFi.disconnect();
const options = await $NETWORK.wifi();

const monitor = new WiFi({
	ssid: {
		[Symbol.toPrimitive]() {
			return options.ssid;
		}
	},
	password: {
		[Symbol.toPrimitive]() {
			return options.password;
		}
	}
}, (msg, value) => {
	if (WiFi.gotIP === msg)
		$DONE();
});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
