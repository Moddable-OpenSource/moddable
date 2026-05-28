/*---
description: a second scan throws while one is in progress
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});
wifi.scan({onFound() {}});

assert.throws(Error, () => {
	const other = new WiFi({});
	try { other.scan({onFound() {}}); }
	finally { other.close(); }
}, "second scan should throw");

wifi.close();
