/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/890
negative:
  type: SyntaxError
---*/

function main() {
const v2 = new ArrayBuffer();
const v3 = BigInt.fromArrayBuffer(v2);
const v4 = v3 % v3;
$262.gc();
}
main();