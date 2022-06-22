/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/800
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v2 = [1000000.0];
const v5 = new Int32Array(36251);
const v8 = 2147483648n << 1000n;
let {"__proto__":v9,"byteOffset":v10,"constructor":v11,"d":v12,"length":v13,} = "zWhxCXUNhU";
const v15 = 268435456n ** v8;
const v17 = [v12];
const v18 = v2.splice;
const v19 = Reflect.apply(v18,v5,v17);
}
main();
