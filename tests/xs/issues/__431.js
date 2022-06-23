/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/431
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function f() {
var a = [10];
[{}] = a.slice(function () {
}, a.length);
a = a.toString(a);
var De65 = !9007199254740994;
var C44J = +-Infinity;
try {
f();
} catch (e) {
}
}
f();
var iGax = f();
var YeKj = f();
var KetZ = f();
var wPbt = f();
var sz6k = +-2147483649;
var Si7p = !1e+400;

