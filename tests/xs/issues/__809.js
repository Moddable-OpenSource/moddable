/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/809
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v0 = {};
const v3 = 2147483648n << 1000n;
const v5 = 268435456n ** v3;
const v6 = [v0];
const v7 = /\W\S/g;
function* v8(v9,v10) {
}
const v13 = new Uint8Array(3955);
const v14 = [3955,3955,3955,3955];
const v16 = [v8,v7];
const v17 = v14.map;
const v18 = Reflect.apply(v17,v13,v16);
}
main();
