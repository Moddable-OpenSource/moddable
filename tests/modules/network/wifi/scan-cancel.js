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
	return $DONE("unexpected callback");
});
WiFi.scan();

WiFi.scan({}, ap => {
	if (!ap)
		$DONE();
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
