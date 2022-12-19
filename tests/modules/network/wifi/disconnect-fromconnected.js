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

let didConnect = true;
const monitor = new WiFi(options, (msg, value) => {
	try {
		switch (msg) {
			case WiFi.disconnected:
				if (didConnect)
					$DONE();
				break;

			case WiFi.connected:
				didConnect = true;
				WiFi.disconnect();
				break;
		}
	}
	catch (e) {
		$DONE(e);
	}
});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
