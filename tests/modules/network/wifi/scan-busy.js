/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (WiFi.Mode.station !== WiFi.mode) {
	WiFi.mode = WiFi.Mode.station;
	Timer.delay(3000);
}

WiFi.scan({}, ap => {});

assert.throws(Error, () =>
	WiFi.scan({}, ap => {}),
	"already scanning");

WiFi.scan();
