/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/580
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function gc() {
    for (let i = 0; i < 500; i++) {
        let ab = new ArrayBuffer(1518500249 | 1073741823);
    }
}
function opt(obj) {
    for (let i = 0; yjwa; i++) {
    }
    let tmp = { a: 1 };
    gc();
    tmp.__proto__ = {};
    var hedB = escape(null);
    for (let k in tmp) {
        tmp.__proto__ = {};
        gc();
        obj.__proto__ = {};
        var yjwa = i < 500;
        return obj[k];
    }
}
opt({});
