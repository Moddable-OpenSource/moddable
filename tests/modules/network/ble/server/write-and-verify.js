/*---
description: 
flags: [async, module]
---*/

import Helper from "bleServerHelper";


class Test extends Helper {
    constructor() {
        super({testIndex: 2});
        this.location = 7;
    }

    onCharacteristicWritten(characteristic, value) {
        super.onCharacteristicWritten(characteristic, value);

        if ('location' === characteristic.name) {
            this.location = value;
        }
    }

    onCharacteristicRead(characteristic) {
        if ('location' === characteristic.name) {
            return this.location;
        }
    }
}

const test = new Test();