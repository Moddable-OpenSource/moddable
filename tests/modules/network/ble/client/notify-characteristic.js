/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`180D`,
    CHARACTERISTIC: uuid`2A37`
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
        services[0].discoverCharacteristic(CONFIG.CHARACTERISTIC);
    }
    onCharacteristics(characteristics) {
        this.onValue = 0;
        characteristics[0].enableNotifications();        
    }
    onCharacteristicNotification(characteristic, value) {
        try {
            if (!CONFIG.CHARACTERISTIC.equals(characteristic.uuid))
                throw 'uuid mismatch'
            
            const v = new Uint8Array(value)[1];
            if (v != this.onValue++)
                throw 'value mismatch'
            
            if (v >= 2) {
                this.device.close();
                this.close();
                $DONE();
            }
        } catch (error) {
            this.device.close();
            this.close();
            $DONE(error);
        }
    }
}

new Client();