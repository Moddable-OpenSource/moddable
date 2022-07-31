/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE connect/disconnect timeout");

class Scanner extends BLEClient {
    onReady() {
        this.startScanning();
    }
    onDiscovered(device) {
        if (device?.scanResponse?.completeName === $TESTMC.config.ble.client.targetDevice) {
            this.stopScanning();
            this.connect(device);
        }
    }
    onConnected(device) {
        device.close();
        this.close();
        $DONE();
    }
}

let scanner = new Scanner();