/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";
import TextDecoder from "text/decoder";
import TextEncoder from "text/encoder";
import Timer from "timer";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "BLE discovery timeout");

const CONFIG = {
    SERVICE_UUID: uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`,
    WRITE_CHARACTERISTIC: uuid`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`,
    NOTIFY_CHARACTERISTIC: uuid`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`,
    VALUES: ['hello', 'world', 'Moddable', 'test']
};

class Client extends BLEClient {
    onReady() {
        this.startScanning();
        this.encoder = new TextEncoder();
        this.decoder = new TextDecoder();
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
        services[0].discoverAllCharacteristics();
    }
    onCharacteristics(characteristics) {
        let writeChar = characteristics.find(element => CONFIG.WRITE_CHARACTERISTIC.equals(element.uuid));
        let readChar = characteristics.find(element => CONFIG.NOTIFY_CHARACTERISTIC.equals(element.uuid));

        readChar.enableNotifications();
        this.on = 0;
        this.match = 0;
        this.timer = Timer.repeat( id => {
            writeChar.writeWithoutResponse((this.encoder.encode(CONFIG.VALUES[this.on++])).buffer);
            if (this.on == CONFIG.VALUES.length)
                Timer.clear(this.timer);
        }, 500);
    }
    onCharacteristicNotification(characteristic, value) {
        try {
            if (!CONFIG.NOTIFY_CHARACTERISTIC.equals(characteristic.uuid))
                throw 'uuid mismatch'
            
            const v = this.decoder.decode(value);
            if (v != CONFIG.VALUES[this.on - 1])
                throw 'value mismatch'
            
            if (this.on == CONFIG.VALUES.length) {
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