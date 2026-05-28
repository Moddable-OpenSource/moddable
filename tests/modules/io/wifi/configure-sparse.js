/*---
description: configure with only hostname does not perturb DHCP state
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
wifi.configure({hostname: "moddable-sparse-test"});
assert.sameValue(wifi.connection <= 200, true, "hostname-only configure should not advance connection state");
wifi.close();
