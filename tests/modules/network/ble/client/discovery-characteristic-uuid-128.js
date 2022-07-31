/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`,
    EXPECTED_CHARACTERISTICS: [uuid`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`, uuid`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`, uuid`6E400004-B5A3-F393-E0A9-E50E24DCCA9E`]
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
        services[0].discoverCharacteristic(CONFIG.EXPECTED_CHARACTERISTICS[0]);
    }
    onCharacteristics(characteristics) {
        this.device.close();
        this.close();
        try {
            if (characteristics.length !== 1)
                throw 'wrong number of characteristics'
            
            if (!(characteristics[0].uuid.equals(CONFIG.EXPECTED_CHARACTERISTICS[0])) )
                throw 'characteristics UUIDs do not match'

            $DONE();
        } catch (error) {
            $DONE(error);
        }        
    }
}

new Client();