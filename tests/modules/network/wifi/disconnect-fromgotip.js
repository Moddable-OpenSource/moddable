/*---
description: 
flags: [async, module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (WiFi.Mode.station !== WiFi.mode) {
	WiFi.mode = WiFi.Mode.station;
	Timer.delay(3000);
}
WiFi.disconnect();
const options = await $NETWORK.wifi();

let didGetIP = true;
const monitor = new WiFi(options, (msg, value) => {
	try {
		switch (msg) {
			case WiFi.disconnected:
				if (didGetIP)
					$DONE();
				break;

			case WiFi.gotIP:
				didGetIP = true;
				WiFi.disconnect();
				break;
		}
	}
	catch (e) {
		$DONE(e);
	}
});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
