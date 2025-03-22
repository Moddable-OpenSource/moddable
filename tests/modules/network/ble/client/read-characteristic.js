/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`180D`,
    CHARACTERISTIC: uuid`2A38`,
    EXPECTED_VALUE: 1
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
		if (!services.length) {
			$DONE("no services discovered");
			return;
		}
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
                throw new Error('uuid mismatch');
            
            if (value != CONFIG.EXPECTED_VALUE)
                throw new Error('value mismatch');
                
            $DONE();
        } catch (error) {
            $DONE(error);
        }
    }
}

new Client();
