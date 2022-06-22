/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/799
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
function v0(v1,v2) {
    'use strict';
    let v3 = "symbol";
    let v4 = Int32Array;
    let v5 = /\W\S/muygi;
    for (let v9 = 0; v9 < 1139733009; v9++) {
        ({"EPSILON":v3,"e":v9,"flags":v9,"multiline":v9,"source":v4,"unicode":v5,} = v5);
    }
    const v12 = new Uint8Array(6504);
    const v13 = v0();
}
const v18 = 2147483648n << 1000n;
const v20 = 268435456n ** v18;
const v21 = [129,129,129];
const v22 = new Promise(v0);
}
main();
