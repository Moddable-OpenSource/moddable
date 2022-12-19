/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/789
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v1 = {};
const v2 = [v1,v1,36251,v1,v1];
try {
    function* v3(v4,v5) {
        'use strict';
    }
    const v8 = new Int32Array(36251);
    const v9 = v8.reduce(v3);
} catch(v10) {
    v2[4096] += v10;
} finally {
}
}
main();
