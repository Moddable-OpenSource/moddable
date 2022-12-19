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
    EXPECTED_DESCRIPTORS: [uuid`2908`, uuid`6E400027-B5A3-F393-E0A9-E50E24DCCA9E`],
    EXPECTED_VALUES: [[3,5], [7,9]]
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
        this.count = 0;
        for (let i = 0; i < descriptors.length; i++)
            descriptors[i].readValue();
    }
    onDescriptorValue(descriptor, value) {
        const view = new Uint8Array(value);
        const index = CONFIG.EXPECTED_DESCRIPTORS.findIndex(element => element.equals(descriptor.uuid));
        
        try {
            if (!CONFIG.EXPECTED_VALUES[index].every( (element, i) => {return element == view[i]})) {
                throw 'value mismatch'
            }

            this.count++;
            if (this.count >= CONFIG.EXPECTED_DESCRIPTORS.length) {
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