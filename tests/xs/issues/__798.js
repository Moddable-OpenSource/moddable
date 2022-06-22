/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/798
flags: [noStrict]
negative:
  type: RangeError
---*/

function main() {
async function* v1(v2,v3) {
    return resourceName;
}
const v7 = Date();
function v8(v9,v10) {
    const v12 = [];
    const v13 = Reflect.apply(v9,v7,v12);
    function v14(v15,v16) {
        const v18 = `object${v13}toString${Date}number${v16}65536${Number}9007199254740992`;
        with (Object) {
            toString = v9;
        }
    }
    const v19 = v8(v14);
}
const v21 = new Promise(v8);
}
main();
