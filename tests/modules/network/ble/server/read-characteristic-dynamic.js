/*---
description: 
flags: [async, module]
---*/

import Helper from "bleServerHelper";


class Test extends Helper {
    constructor() {
        super({ testIndex: 6 });
    }

    onCharacteristicRead(characteristic) {
        if (characteristic.name === "location") {
            return Math.floor(Math.random() * 100) + 1;
        } else {
            return 0;
        }
    }
}

const test = new Test();