/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`180D`,
    NUM_SERVICES: 3
};

class Client extends BLEClient {
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
        device.discoverAllPrimaryServices();
    }
    onServices(services) {
        this.close();
        if (services.length === CONFIG.NUM_SERVICES) {
            $DONE();
        } else {
            $DONE('discoverAllPrimaryServices failed');
        }
    }
}

new Client();