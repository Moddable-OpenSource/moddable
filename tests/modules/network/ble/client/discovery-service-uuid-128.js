/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    UUID_128: uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`,
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
        device.discoverPrimaryService(CONFIG.UUID_128);
    }
    onServices(services) {
        this.device.close();
        this.close();
        try {
            if (services.length !== 1)
                throw 'discoverPrimaryService 128-bit UUID failed on services length';
            if (!services[0].uuid.equals(CONFIG.UUID_128))
                throw 'discoverPrimaryService 128-bit UUID failed on UUID match';
            $DONE();
        } catch (error) {
            $DONE(error);
        }
    }
}

new Client();