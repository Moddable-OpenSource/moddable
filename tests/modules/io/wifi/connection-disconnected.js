/*---
description: connection property reports <= 200 (disconnected) when not connected
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
assert.sameValue(wifi.connection <= 200, true, "disconnected should report <= 200");
wifi.close();
