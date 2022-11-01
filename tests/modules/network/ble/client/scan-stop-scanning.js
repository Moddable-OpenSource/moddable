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
                delete this.timer;
                this.finish();
            }, 3_000);
        } else {
            this.finish("received scan result after calling stopScanning");
        }
    }
    finish(error) {
        if (this.timer)
            Timer.clear(this.timer);
        this.close();
        $DONE(error);
    }
}

let scanner = new Scanner();