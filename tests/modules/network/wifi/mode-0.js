/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";

WiFi.mode = WiFi.Mode.none;
const options = await $NETWORK.wifi();

assert.throws(Error, () => {
	WiFi.scan({}, ap => {
		throw new RangeError("unexpected");
	});
}, () => {}, "scan with Wi-Fi mode none");

assert.throws(Error, () => {
	new WiFi(options, (msg, value) => {
		throw new RangeError("unexpected");
	});
}, () => {}, "connect with Wi-Fi mode none");
