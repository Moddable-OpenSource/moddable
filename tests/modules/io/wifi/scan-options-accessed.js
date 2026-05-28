/*---
description: scan reads option values from the provided options object
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const wifi = new WiFi({});

let channelAccessed = 0;
const onFound = () => {};

wifi.scan({
	onFound,
	get channel() {
		channelAccessed += 1;
		return 1;
	}
});

assert.sameValue(channelAccessed > 0, true, "scan should read channel option");

wifi.close();
