/*---
description: WiFi can be constructed and closed
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";

const wifi = new WiFi({});
assert.sameValue(typeof wifi.close, "function", "close method");
assert.sameValue(typeof wifi.scan, "function", "scan method");
assert.sameValue(typeof wifi.connect, "function", "connect method");
assert.sameValue(typeof wifi.disconnect, "function", "disconnect method");
assert.sameValue(typeof wifi.configure, "function", "configure method");
wifi.close();
