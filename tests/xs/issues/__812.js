/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/812
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
function v0(v1,v2) {
    function* v4(v5,v6) {
    }
    const v9 = new Int32Array(36251);
    try {
        const v10 = v9.reduce(v4);
    } catch(v11) {
    }
    let v12 = Set;
    function v13(v14,v15) {
        const v17 = Object.defineProperty();
    }
    const v19 = new Promise(v13);
    let v21 = {};
    let v22 = /4\Dp+/sgi;
    for (let v23 = -536870912; v23 >= -536870912; v23 = v23 + 0) {
        ({"EPSILON":v23,"e":v23,"flags":v21,"multiline":v23,"source":v12,"unicode":v22,} = v22);
    }
    const v24 = v0();
}
const v26 = new Promise(v0);
}
main();
