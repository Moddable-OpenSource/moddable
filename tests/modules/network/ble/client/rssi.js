/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE rssi timeout");

const CONFIG = {
    SERVICE_UUID: uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
};

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
		this.device = device;
        device.discoverPrimaryService(CONFIG.SERVICE_UUID);
    }
    onServices(services) {
        this.device.readRSSI();
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