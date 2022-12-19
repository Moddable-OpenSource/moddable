/*---
description: 
flags: [async, module]
---*/

import Helper from "bleServerHelper";


class Test extends Helper {
    constructor() {
        super({ testIndex: 7 });
    }
}

const test = new Test();