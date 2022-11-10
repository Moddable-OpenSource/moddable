/*---
description: 
flags: [async, module]
---*/

// Inspired by an example provided by @brenocastrocardoso in Moddable SDK issue #955 (https://github.com/Moddable-OpenSource/moddable/issues/955)

import Timer from "timer";

let count = 0;

Timer.set(() => {
    Timer.clear(repeat);
    Timer.clear(timer);

    if (count === 33) {
        $DONE();
    } else {
        $DONE(`wrong count: ${count}`);
    }
}, 1_000);


let timer = Timer.set(() => {
    count++;
    Timer.schedule(timer);
}, 10);

let repeat = Timer.set(() => {
    Timer.schedule(timer, 10);
}, 30, 30);