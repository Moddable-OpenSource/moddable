/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/804
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v3 = 2147483648n << 1000n;
const v5 = 268435456n ** v3;
const v6 = [-2874546747,-2874546747,-2874546747,-2874546747];
const v8 = [-123860.30242099147,-123860.30242099147,-123860.30242099147,-123860.30242099147];
const v9 = /1N\wc.*/ug;
const v10 = {};
const v11 = [v10];
function* v12(v13,v14,v15,v16,v17) {
}
const v20 = new Uint8ClampedArray(60800);
const v22 = [v12,Reflect];
const v23 = v11.flatMap;
const v24 = Reflect.apply(v23,v20,v22);
}
main();
