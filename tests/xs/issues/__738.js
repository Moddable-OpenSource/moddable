/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/738
flags: [onlyStrict]
includes: [compareArray.js]
---*/

let arr = [
    1.5,
    2.5
];
let res = (arr.slice(0, {
    valueOf: function () {
        arr.length = 0;
        return 2;
    }
}));

assert.compareArray(res, [undefined,undefined], "[undefined,undefined]");
