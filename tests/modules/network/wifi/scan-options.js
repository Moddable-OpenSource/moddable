/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (1 !== WiFi.mode) {
	WiFi.mode = 1;
	Timer.delay(3000);
}

let hidden = false, channel = false, active = false;
class Options {
	static get hidden() {
		hidden = true;
		return false;
	} 
	static get channel() {
		channel = true;
		return 1;
	} 
	static get active() {
		active = true;
		return 1;
	} 
}

WiFi.scan(Options, ap => {});

assert.sameValue(hidden, true, "hidden property not read");
assert.sameValue(channel, true, "channel property not read");
assert.sameValue(active, true, "active property not read");

WiFi.scan();

assert.throws(Error, () => WiFi.scan("123", () => {}), "options must be object, not string");
assert.throws(Error, () => WiFi.scan(12, () => {}), "options must be object, not number");
assert.throws(Error, () => WiFi.scan(1n, () => {}), "options must be object, not BigInt");

assert.throws(Error, () => WiFi.scan({}));

WiFi.scan();

$TESTMC.timeout($TESTMC.wifiScanTimeout);
