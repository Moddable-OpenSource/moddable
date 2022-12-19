/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/974
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
function* v0(v1,v2) {
    'use strict';
}
const v5 = new Int32Array(36251);
try {
    const v6 = v5.reduce(v0);
} catch(v7) {
}
do {
    async function v10(v11,v12,v13) {
    }
    const v14 = {};
    const v15 = {"apply":v10,"call":v10,"construct":v10,"defineProperty":v10,"deleteProperty":v10,"get":v10,"getOwnPropertyDescriptor":v10,"has":v10,"ownKeys":v10,"setPrototypeOf":v10};
    const v17 = new Proxy(v14,v15);
    Reflect.__proto__ = v17;
} while (0 < 7);
}
main();
