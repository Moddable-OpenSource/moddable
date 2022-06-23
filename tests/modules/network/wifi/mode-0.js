/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";

WiFi.mode = 0;
const options = await $NETWORK.wifi();

assert.throws(Error, () => {
	WiFi.scan({}, ap => {
		throw new RangeError("unexpected");
	});
}, () => {}, "scan with Wi-Fi disabled");

assert.throws(Error, () => {
	new WiFi(options, (msg, value) => {
		throw new RangeError("unexpected");
	});
}, () => {}, "connect with Wi-Fi disabled");
