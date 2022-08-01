/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (1 !== WiFi.mode) {
	WiFi.mode = 1;
	Timer.delay(3000);
}

WiFi.scan({}, ap => {});

assert.throws(Error, () =>
	WiFi.scan({}, ap => {}),
	"already scanning");

WiFi.scan();
