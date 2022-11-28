/*---
description: 
flags: [async, module]
---*/

import Helper from "bleServerHelper";


class Test extends Helper {
    constructor() {
        super({ testIndex: 5 });
    }
}

const test = new Test();