/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

let count = 0;

let timer = Timer.set(id => {
    count++;
}, 10, 30);

Timer.set(() => {
    Timer.clear(timer);
    if (count === 34) {
        $DONE();
    } else {
        $DONE(`wrong count: ${count}`);
    }
}, 1000);