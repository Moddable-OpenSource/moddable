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

let once = false;;
WiFi.scan({}, ap => {
	if (once) return;
	once = true;
	$DONE();
});

$TESTMC.timeout($TESTMC.wifiScanTimeout);
