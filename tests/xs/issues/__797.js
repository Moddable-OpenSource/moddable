/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/797
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v2 = Symbol.split;
const v3 = [-1000000000000.0,-1000000000000.0,-1000000000000.0];
const v6 = 2147483648n << 1000n;
const v8 = 268435456n ** v6;
const v9 = {};
const v10 = [v9,v9,v9,v3,v9];
function v11(v12,v13) {
    'use strict';
    const v14 = [];
    function* v17(v18,v19,...v20) {
        const v21 = 36251;
        const v22 = 9;
        const v23 = -734507737 & v2;
        const v24 = [v23,v14];
        const v25 = v18(Object,v23,v12);
    }
    const v26 = v17(-734507737);
    const v27 = v11();
}
const v29 = new Promise(v11);
}
main();
