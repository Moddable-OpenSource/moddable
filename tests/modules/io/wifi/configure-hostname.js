/*---
description: configure reads the hostname value and rejects RFC-illegal lengths
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";

const wifi = new WiFi({});

// Spy: confirms configure actually accesses the hostname value.
let accessed = 0;
const spy = {
	toString() {
		accessed += 1;
		return "moddable-test";
	}
};
wifi.configure({hostname: spy});
assert.sameValue(accessed > 0, true, "configure should read the hostname value");

// Hostnames longer than RFC 1035's 63-octet label limit must be rejected.
const tooLong = "a".repeat(64);
assert.throws(Error, () => wifi.configure({hostname: tooLong}), "hostname longer than 63 octets should be rejected");

wifi.close();
