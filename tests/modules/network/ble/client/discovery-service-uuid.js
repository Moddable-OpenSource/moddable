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
        device.discoverPrimaryService(CONFIG.SERVICE_UUID);
    }
    onServices(services) {
        this.device.close();
        this.close();
        try {
            if (services.length !== 1)
                throw 'discoverPrimaryService failed on services length';
            if (!services[0].uuid.equals(CONFIG.SERVICE_UUID))
                throw 'discoverPrimaryService failed on UUID match';
            $DONE();
        } catch (error) {
            $DONE(error);
        }
    }
}

new Client();