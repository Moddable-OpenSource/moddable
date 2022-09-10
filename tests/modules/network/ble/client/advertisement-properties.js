/*---
description: 
flags: [async, module]
---*/

import BLEClient from "bleclient";
import {uuid} from "btutils";

$TESTMC.timeout($TESTMC.config.ble.client.timeout, "target BLE peripheral not found");

class Scanner extends BLEClient {
    onReady() {
        this.startScanning();
    }
    onDiscovered(device) {
        if (device?.scanResponse?.completeName === $TESTMC.config.ble.client.targetDevice) {
            this.close();

            try {
                if (device.scanResponse.flags !== 6) 
                    throw "incorrect flags";
                
                const uuidList = device.scanResponse.incompleteUUID16List;
                if (uuidList === undefined)
                    throw "incompleteUUID16List is missing";
                if (!uuidList.length === 1)
                    throw "bad UUID list length";
                if (!uuidList.some(element => element.equals(uuid`180D`)))
                    throw "missing UUID 180D";

                const msd = device.scanResponse.manufacturerSpecific;
                if (msd === undefined)
                    throw "manufacturer-specfic data is missing";
                if (msd.identifier !== 0x03)
                    throw "wrong manufacturer-specific data identifier";
                if (typeof msd.data !== "object" || Object.keys(msd.data).length !== 3)
                    throw "incorrect manufacturer-specific data format";
                if (msd.data[0] !== 1 || msd.data[1] !== 2 || msd.data[2] !== 3)
                    throw "unexpected manufacturer-specific data";

                $DONE();
            } catch (error) {
                $DONE(error);
            }
        }
    }
}

let scanner = new Scanner();