/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/803
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v3 = 2147483648n << 1000n;
const v5 = 268435456n ** v3;
const v6 = [257];
const v9 = new Int16Array(3155);
const v11 = [-705113.8005727688,-705113.8005727688,v9,-705113.8005727688];
const v14 = JSON["stringify"](v11);
const v15 = JSON.parse(v14);
}
main();
