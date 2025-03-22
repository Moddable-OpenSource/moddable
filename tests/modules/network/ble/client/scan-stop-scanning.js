/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import Timer from "timer";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE test timeout");

class Scanner extends BLEClient {
    onReady() {
        this.isScanning = true;
        this.startScanning();
    }
    onDiscovered(device) {
        if (this.isScanning) {
            this.stopScanning();
            this.isScanning = false;
            this.timer = Timer.set(id => {
				this.close();
				$DONE();
            }, 3_000);
        } else {
            Timer.clear(this.timer);
			this.close();
            $DONE("received scan result after calling stopScanning");
        }
    }
}

let scanner = new Scanner();
