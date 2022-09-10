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
        device.readRSSI();
    }

    onRSSI(device, rssi) {
        device.close();
        this.close();

        try {
            if (typeof rssi !== "number")
                throw "non-numeric rssi"
            if (rssi > 0)
                throw "unexpected positive rssi"

            $DONE();
        } catch (error) {
            $DONE(error);
        }
    }
}

let scanner = new Scanner();