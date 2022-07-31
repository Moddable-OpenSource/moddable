/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";
import TextDecoder from "text/decoder";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`,
    CHARACTERISTIC: uuid`6E400004-B5A3-F393-E0A9-E50E24DCCA9E`,
    EXPECTED_VALUE: "test123"
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
        characteristics[0].readValue();        
    }
    onCharacteristicValue(characteristic, value) {
        this.device.close();
        this.close();
        try {
            if (!CONFIG.CHARACTERISTIC.equals(characteristic.uuid))
                throw 'uuid mismatch'
            
            if (new TextDecoder().decode(value) !== CONFIG.EXPECTED_VALUE)
                throw 'value mismatch'
                
            $DONE();
        } catch (error) {
            $DONE(error);
        }
    }
}

new Client();