/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";
import Timer from "timer";

WiFi.mode = WiFi.Mode.none;

assert.throws(Error, () => WiFi.scan({}, () => {}), "cannot scan when wi-fi disabled");
