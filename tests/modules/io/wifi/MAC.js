/*---
description: MAC property returns the MAC-formatted address regardless of connection state
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
assert.sameValue(wifi.connection <= 200, true, "starts disconnected (<= 200)");

const mac = wifi.MAC;
// Spec format for MAC: zz:zz:zz:zz:zz:zz, lowercase hex, colon-separated.
assert.sameValue(typeof mac, "string", "MAC is a string");
assert.sameValue(mac.length, 17, "MAC is 17 chars");
assert(/^[0-9a-f]{2}(:[0-9a-f]{2}){5}$/.test(mac), "MAC has expected format: " + mac);

wifi.close();
