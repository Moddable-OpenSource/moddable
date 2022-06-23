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
const monitor = new WiFi(options, (msg, value) => {
	try {
		switch (msg) {
			case WiFi.disconnected:
				assert.sameValue(state, WiFi.disconnected, "during connect, disconnected only expected when disconnected");
				break;

			case WiFi.connected:
				assert.sameValue(state, WiFi.disconnected, "must be disconnected to receive connected");
				break;
				
			case WiFi.gotIP:
				assert.sameValue(state, WiFi.connected, "must be connected to receive gotIP");
				$DONE();
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

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
