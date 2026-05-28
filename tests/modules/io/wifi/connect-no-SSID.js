/*---
description: connect without SSID throws (SSID is required, BSSID is optional)
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
// Pass other valid optional fields to isolate that the failure is the missing SSID.
assert.throws(Error, () => wifi.connect({password: "anything", channel: 1}), "connect without SSID should throw");
wifi.close();
