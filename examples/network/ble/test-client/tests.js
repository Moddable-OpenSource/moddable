/*
 * Copyright (c) 2022 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {uuid} from "btutils";

class Test {

    constructor(onCompleteCallback) {
        this.callback = onCompleteCallback;
        this.done = false;
    }

    complete(code = "okay") {
        if (!this.done) {
            this.callback(code);
            this.done = true;
        }
    }

    startTest(device) {}
    onServices(services) {}
    onCharacteristics(characteristics) {}
    onCharacteristicNotification(characteristic, value) {}
    onDescriptors() {}
    onCharacteristicValue(characteristic, value) {}
    onDescriptorValue() {}
}

class ConnectDisconnectTest extends Test {
    startTest() {
        super.complete();
    }
}


const GAP_UUID = uuid`1800`;
const DEVICE_NAME_UUID = uuid`2A00`;
const APPEARANCE_UUID = uuid`2A01`;

class ReadGAP extends Test {
    startTest(device) {
        device.discoverPrimaryService(GAP_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(GAP_UUID))
            services[0].discoverAllCharacteristics();
    }
    onCharacteristics(characteristics) {
        for (const char of characteristics) {
            if (char.uuid.equals(DEVICE_NAME_UUID)) {
                this.nameChar = char;
            } else if (char.uuid.equals(APPEARANCE_UUID)) {
                this.appearanceChar = char;
            }
        }

        if (this.nameChar === undefined || this.appearanceChar === undefined) {
            this.complete("missing characteristic");
        } else {
            this.nameChar.readValue();
            // this.appearanceChar.readValue();
        }
    }

    onCharacteristicValue(characteristic, value) {
        trace(`found ${characteristic.uuid.toString()}, ${value}\n`);
        if (characteristic === this.nameChar && value === "testmc Server")  {
            this.nameOkay = true;
            this.appearanceChar.readValue();
        }

        if (characteristic === this.appearanceChar && value === 3) {
            this.appearanceOkay = true;
        }
            
        if (this.appearanceOkay && this.nameOkay) {
            super.complete();
        }
    }
}

const HEART_RATE_SERVICE_UUID = uuid`180D`;
const UART_SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;

class WriteAndVerify extends Test {
    startTest(device) {
        this.state = 0;
        device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(HEART_RATE_SERVICE_UUID))
            services[0].discoverCharacteristic(uuid`2A38`);
    }
    onCharacteristics(characteristics) {
        if (characteristics?.[0]?.uuid?.equals(uuid`2A38`)) {
            this.char = characteristics[0];
            this.char.readValue();
        }
    }
    onCharacteristicValue(characteristic, value) {
        if (this.state === 0 && value === 7) {
            this.state = 1;
            this.char.writeWithoutResponse(13);
            this.char.readValue();
        } else if (this.state === 1 && value === 13) {
            super.complete();
        } else {
            super.complete("incorrect value");
        }
    }
}

class Notifications extends Test {
    startTest(device) {
        device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(HEART_RATE_SERVICE_UUID))
            services[0].discoverCharacteristic(uuid`2A37`);
    }
    onCharacteristics(characteristics) {
        if (characteristics?.[0]?.uuid?.equals(uuid`2A37`)) {
            this.char = characteristics[0];
            this.char.enableNotifications();
            this.state = 0;
        }
    }
    onCharacteristicNotification(characteristic, value) {
        if (value[1] !== (this.state++ * 7)) {
            super.complete("incorrect value");
        }

        if (this.state == 5)
            super.complete();
    }
}

class ReadDescriptor extends Test {
    startTest(device) {
        device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(HEART_RATE_SERVICE_UUID))
            services[0].discoverCharacteristic(uuid`2A4D`);
    }
    onCharacteristics(characteristics) {
        if (characteristics?.[0]?.uuid?.equals(uuid`2A4D`)) {
            characteristics[0].discoverAllDescriptors();
        }
    }
    onDescriptors(descriptors) {
        if (descriptors.length !== 1) {
            super.complete("descriptors not found");
        } else {
            descriptors[0].readValue();
        }
    }
    onDescriptorValue(descriptor, value) {
        if (descriptor?.uuid?.equals(uuid`2908`) && value === 3) {
            super.complete();
        } else {
            super.complete("incorrect descriptor data");
        }
    }
}

class ReadStaticCharacteristic extends Test {
    startTest(device) {
        device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(HEART_RATE_SERVICE_UUID))
            services[0].discoverCharacteristic(uuid`2A4D`);
    }
    onCharacteristics(characteristics) {
        if (characteristics?.[0]?.uuid?.equals(uuid`2A4D`)) {
            this.char = characteristics[0];
            this.char.readValue();
        }
    }
    onCharacteristicValue(characteristic, value) {
        if (value === 25) {
            super.complete();
        } else {
            super.complete("incorrect value");
        }
    }
}

class ReadDynamicCharacteristic extends Test {
    startTest(device) {
        device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(HEART_RATE_SERVICE_UUID))
            services[0].discoverCharacteristic(uuid`2A38`);
    }
    onCharacteristics(characteristics) {
        if (characteristics?.[0]?.uuid?.equals(uuid`2A38`)) {
            this.char = characteristics[0];
            this.char.readValue();
            this.values = [];
        }
    }
    onCharacteristicValue(characteristic, value) {
        this.values.push(value);

        if (this.values.length > 5) {
            if (this.values.every(value => value === this.values[0])) {
                super.complete("all values the same");
            } else {
                super.complete();
            }
        } else {
            this.char.readValue();
        }
    }
}

class AdvertisementProperties extends Test {
    startTest(device, scanResponse) {

        if (scanResponse?.flags !== 6) 
            return super.complete("incorrect flags");

        if (scanResponse?.completeName !== "testmc Server")
            return super.complete("incorrect complete name")

        const data = scanResponse?.manufacturerSpecific?.data;
        if (!(data instanceof Uint8Array) || data.length < 1)
            return super.complete("invalid manufacturer-specific data");

        if (data[0] !== 7)
            return super.complete("wrong manufacturer-specific data");

        super.complete();
    }
}

class Whitelist extends Test {
    startTest(device) {
        device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
    }
    onServices(services) {
        if (services?.[0]?.uuid?.equals(HEART_RATE_SERVICE_UUID))
            services[0].discoverCharacteristic(uuid`2A38`);
    }
    onCharacteristics(characteristics) {
        if (characteristics?.[0]?.uuid?.equals(uuid`2A38`)) {
            this.char = characteristics[0];
            this.char.writeWithoutResponse(Math.floor(Math.random() * 128) + 1);
        }
    }
}

export default [
    ConnectDisconnectTest,      //0
    ReadGAP,                    //1
    WriteAndVerify,             //2
    Notifications,              //3
    ReadDescriptor,             //4
    ReadStaticCharacteristic,   //5
    ReadDynamicCharacteristic,  //6
    AdvertisementProperties,    //7
    Whitelist                   //8
]