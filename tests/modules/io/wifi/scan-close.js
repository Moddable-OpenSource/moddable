/*---
description: closing while scanning must not invoke onFound or onComplete on the closed instance
flags: [async, module]
---*/

import WiFi from "embedded:network/interface/wifi";
import Timer from "timer";
import resetWiFi from "./wifiReset_FIXTURE.js";

resetWiFi();

const doomed = new WiFi({});
doomed.scan({
	onFound() { $DONE("onFound on closed instance"); },
	onComplete() { $DONE("onComplete on closed instance"); }
});
doomed.close();

// Most platforms can't actually cancel a scan; poll for scan availability
// then run a control scan whose clean completion proves the doomed scan's
// callbacks were never delivered.
const control = new WiFi({});

let finished = false;
function finish() {
	if (finished) return;
	finished = true;
	try { control.close(); } catch {}
	$DONE();
}

function tryControlScan() {
	try {
		control.scan({
			onFound: finish,        // close + $DONE on first hit (stress: close from within callback)
			onComplete: finish      // fallback for the rare "no APs in range"
		});
	}
	catch {
		Timer.set(tryControlScan, 500);
	}
}

tryControlScan();

$TESTMC.timeout($TESTMC.wifiScanTimeout);
