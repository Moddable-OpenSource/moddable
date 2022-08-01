/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/796
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
function v0(v1,v2,v3,v4) {
    'use strict';
    function* v5(v6,v7) {
        'use strict';
    }
    const v10 = new Int32Array(36251);
    try {
        const v11 = v10.reduce(v5);
    } catch(v12) {
    }
}
const v13 = /A*/gi;
const v16 = "9ZQ9HPSlRT"["replace"](v13,v0);
}
main();
