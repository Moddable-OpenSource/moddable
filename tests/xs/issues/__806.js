/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/806
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v1 = [9007199254740991];
function* v2(v3,v4) {
    'use strict';
}
const v7 = new Int32Array(36251);
try {
    const v8 = v7.reduce(v2);
} catch(v9) {
}
const v10 = delete v1[3308989510];
}
main();
