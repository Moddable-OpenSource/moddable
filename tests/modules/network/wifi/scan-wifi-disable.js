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

WiFi.scan({}, ap => {
	if (ap)
		return $DONE("no access point expected");

	$DONE();
});

WiFi.mode = WiFi.Mode.none;

$TESTMC.timeout($TESTMC.wifiScanTimeout);
