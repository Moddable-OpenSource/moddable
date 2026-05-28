/*---
description: SSID/BSSID/RSSI/channel/address are undefined when not connected
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
assert.sameValue(wifi.connection <= 200, true, "starts disconnected (<= 200)");
assert.sameValue(wifi.SSID, undefined, "SSID is undefined when not connected");
assert.sameValue(wifi.BSSID, undefined, "BSSID is undefined when not connected");
assert.sameValue(wifi.RSSI, undefined, "RSSI is undefined when not connected");
assert.sameValue(wifi.channel, undefined, "channel is undefined when not connected");
assert.sameValue(wifi.address, undefined, "address is undefined when not connected");
wifi.close();
