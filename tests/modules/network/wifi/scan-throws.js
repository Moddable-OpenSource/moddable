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
