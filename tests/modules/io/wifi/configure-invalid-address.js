/*---
description: configure throws RangeError on malformed static IP strings
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";

const wifi = new WiFi({});

assert.throws(RangeError, () => wifi.configure({
	static: {address: "not.an.ip.address", mask: "255.255.255.0", gateway: "192.168.1.1"}
}), "invalid address");

assert.throws(RangeError, () => wifi.configure({
	static: {address: "192.168.1.2", mask: "garbage", gateway: "192.168.1.1"}
}), "invalid mask");

assert.throws(RangeError, () => wifi.configure({
	static: {address: "192.168.1.2", mask: "255.255.255.0", gateway: "999.999.999.999"}
}), "invalid gateway");

wifi.close();
