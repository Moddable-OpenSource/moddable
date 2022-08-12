/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "target BLE peripheral not found");

class Scanner extends BLEClient {
    onReady() {
        this.startScanning();
    }
    onDiscovered(device) {
        if (device?.scanResponse?.completeName === $TESTMC.config.ble.client.targetDevice) {
            this.close();
            $DONE();
        }
    }
}

let scanner = new Scanner();