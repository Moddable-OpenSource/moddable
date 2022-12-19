/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/811
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
let v0 = isNaN;
const v2 = Date();
function v3(v4,v5) {
    const v7 = [];
    const v8 = Reflect.apply(v4,v2,v7);
    function v9(v10,v11) {
        v0 &&= v4;
        for (const v12 in v7) {
        }
    }
    const v13 = v3(v9);
}
const v15 = new Promise(v3);
}
main();
