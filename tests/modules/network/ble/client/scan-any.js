/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "no BLE peripherals found");

class Scanner extends BLEClient {
    onReady() {
        this.startScanning();
    }
    onDiscovered(device) {
        this.close();
        $DONE();
    }
}

let scanner = new Scanner();
