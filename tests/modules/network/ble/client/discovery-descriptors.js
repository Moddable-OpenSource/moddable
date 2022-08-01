/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`,
    CHARACTERISTIC: uuid`6E400004-B5A3-F393-E0A9-E50E24DCCA9E`,
    EXPECTED_DESCRIPTORS: [uuid`2908`, uuid`6E400027-B5A3-F393-E0A9-E50E24DCCA9E`]
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
        characteristics[0].discoverAllDescriptors();
    }
    onDescriptors(descriptors) {
        this.device.close();
        this.close();
        try {
            if (descriptors.length !== CONFIG.EXPECTED_DESCRIPTORS.length)
                throw 'wrong number of descriptors'
            
            let matches = 0;
            for (let i = 0; i < descriptors.length; i++)
                if ( CONFIG.EXPECTED_DESCRIPTORS.some( (element) => element.equals(descriptors[i].uuid) ) )
                    matches++;
            if (matches !== CONFIG.EXPECTED_DESCRIPTORS.length)
                throw 'descriptor UUIDs do not match'

            $DONE();
        } catch (error) {
            $DONE(error);
        }        
    }
}

new Client();