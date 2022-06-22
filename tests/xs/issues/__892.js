/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/892
negative:
  type: RangeError
---*/

function main() {
const v2 = RangeError();
const v3 = RangeError(v2);
const v5 = "127".padStart(686749.8011888242,v3);
const v7 = eval(v5);
$262.gc();
}
main();