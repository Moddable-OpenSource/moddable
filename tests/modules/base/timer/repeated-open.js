/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

let count = 0;

Timer.repeat(t => {
    if (count < 30) {
        Timer.set(() => {
            count++;
        }, 5);
    } else {
        Timer.clear(t);
        $DONE();
    }
}, 10);
