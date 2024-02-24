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

let count = 0;
WiFi.scan({}, ap => {
	if (!ap) {
		if (0 === count)
			return $DONE("no access point found");
		return $DONE();
	}
	
	count += 1;
	throw new Error;
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
