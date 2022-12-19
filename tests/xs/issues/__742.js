/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/742
flags: [onlyStrict]
---*/

let __v_25055 = null;
let __v_25056 = null;
let __v_25059 = {
    valueOf: function () {
        let __v_25062 = __v_25055.length;
        __v_25055.length = 1;
        return __v_25062;
    }
};
let __v_25060 = [];
for (let __v_25063 = 0; __v_25063 < 1500; __v_25063++) {
    __v_25060.push('' + 0.1);
}
for (let __v_25064 = 0; __v_25064 < 75; __v_25064++) {
   	__v_25055 = __v_25060.slice();
    __v_25056 = __v_25055.slice(0, __v_25059);
}
