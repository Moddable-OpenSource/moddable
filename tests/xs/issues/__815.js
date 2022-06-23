/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/815
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
async function v0(v1,v2,v3,...v4) {
    const v5 = await v2;
    const v7 = new Promise(v0);
    const v8 = v7.finally(v5);
}
const v9 = v0();
}
main();
