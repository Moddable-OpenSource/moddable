/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";
import Timer from "timer";

WiFi.mode = 0;

assert.throws(Error, () => WiFi.scan({}, () => {}), "cannot scan when wi-fi disabled");
