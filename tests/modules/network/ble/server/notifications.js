/*---
description: 
flags: [async, module]
---*/

import Helper from "bleServerHelper";
import Timer from "timer";

class Test extends Helper {
    constructor() {
        super({testIndex: 3});
        this.location = 7;
    }

    onCharacteristicNotifyEnabled(characteristic) {
        if ('bpm' === characteristic.name) {
            let state = 0;
            Timer.repeat(id => {
                const value = [0, state++ * 7];
                this.notifyValue(characteristic, value);
                if (state === 5) 
                    Timer.clear(id);
            }, 1000);
        }
    }
}

const test = new Test();