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

WiFi.scan({}, ap => {
	if (ap)
		return $DONE("no access point expected");

	$DONE();
});

WiFi.mode = 0;

$TESTMC.timeout($TESTMC.wifiScanTimeout);
