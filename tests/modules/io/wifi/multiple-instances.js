/*---
description: two WiFi instances both observe state transitions and continue independently
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();

let aReached500 = false, bReached500 = false;
let done = false;

function check() {
	if (done) return;
	if (!(aReached500 && bReached500)) return;
	done = true;
	try {
		// Both instances observe the same state.
		assert.sameValue(a.connection >= 500, true, "A reports >= 500");
		assert.sameValue(b.connection >= 500, true, "B reports >= 500");
		assert.sameValue(a.address, b.address, "both report the same address");

		// Closing one instance must not affect the other.
		a.close();
		assert.sameValue(b.connection >= 500, true, "B still reports >= 500 after A is closed");
		assert.sameValue(typeof b.address, "string", "B still reports an address after A is closed");

		b.close();
		$DONE();
	}
	catch (e) {
		$DONE(e);
	}
}

const a = new WiFi({
	onChanged(property) {
		if ("connection" === property && a.connection >= 500) {
			aReached500 = true;
			check();
		}
	}
});

const b = new WiFi({
	onChanged(property) {
		if ("connection" === property && b.connection >= 500) {
			bReached500 = true;
			check();
		}
	}
});

// Drive the connection from either instance — both should observe the transitions.
a.connect({SSID: opts.ssid, password: opts.password});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
