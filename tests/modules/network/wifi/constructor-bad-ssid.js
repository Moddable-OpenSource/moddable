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
WiFi.disconnect();
const options = await $NETWORK.wifi();

let state = WiFi.disconnected;
options.ssid = ("BAD-" + options.ssid).slice(0, 31);
const monitor = new WiFi(options, (msg, value) => {
	try {
		switch (msg) {
			case WiFi.disconnected:
				assert.sameValue(state, WiFi.disconnected, "during connect, disconnected only expected when disconnected");
				$DONE();
				break;

			case WiFi.connected:
			case WiFi.gotIP:
				assert(false, "cannot connect to invalid ssid");
				break;

			default:
				throw new Error("unexpected msg: " + msg);
		}
		state = msg;
	}
	catch (e) {
		$DONE(e);
	}
});

$TESTMC.timeout($TESTMC.wifiInvalidConnectionTimeout);
