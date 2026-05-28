/*---
description: closing one WiFi instance from inside its own callback does not suppress the other's
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const opts = await $NETWORK.wifi();

let aDone = false;
let bDone = false;

function maybeDone() {
	if (aDone && bDone)
		$DONE();
}

// Each instance closes itself from inside its own first state-500 callback.
// Test passes only if both callbacks fire — irrespective of delivery order.
const a = new WiFi({
	onChanged(property) {
		try {
			if ("connection" !== property) return;
			if (a.connection < 500) return;
			if (aDone) return;
			aDone = true;
			a.close();
			maybeDone();
		}
		catch (e) {
			$DONE(e);
		}
	}
});

const b = new WiFi({
	onChanged(property) {
		try {
			if ("connection" !== property) return;
			if (b.connection < 500) return;
			if (bDone) return;
			bDone = true;
			b.close();
			maybeDone();
		}
		catch (e) {
			$DONE(e);
		}
	}
});

a.connect({SSID: opts.ssid, password: opts.password});

$TESTMC.timeout($TESTMC.wifiConnectionTimeout);
