/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/791
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v0 = Uint16Array;
const v2 = [64,64];
const v3 = Reflect;
const v4 = WeakSet;
const v5 = Date;
function* v6(v7,v8) {
}
const v11 = new Int32Array(36251);
try {
    const v12 = v11.reduce(v6);
} catch(v13) {
}
const v15 = new Set();
function v16(v17,v18) {
    const v20 = v2["shift"]();
    const v21 = "boolean";
    const v22 = Int8Array;
    const v23 = Array;
    const v24 = v15.delete;
}
const v26 = new Promise(v16);
}
main();
