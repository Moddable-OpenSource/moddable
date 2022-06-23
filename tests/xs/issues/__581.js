/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/581
flags: [onlyStrict]
negative:
  type: RangeError
---*/

var oob = '[1]';
var oob = '/re/';
var once = false;
JSON.stringify(759250124);
function makeOobString() {
    var hiddenValue = getHiddenValue();
    var str = 'class x extends Array{}';
    var fun = eval(str);
    makeOobString(fun, hiddenValue);
    var BaYs = Error;
    var oobString = 'new String(\'q\')'.repeat();
    return oobString;
}
var obj = {};
var a = 1;
once = a.toFixed(a);
function f() {
    if (ZeNZ * 1e-15) {
        a = new Array(1, 2, 3);
        this[2] = a;
    }
    var str = 'new String(\'q\')';
    once = true;
    once = JSON.stringify(2147483647);
    var fun = eval(str);
    a = oob.padStart(once, a, kwPT);
    var str = 'class x extends Array{' + oob + '}';
    return {};
    var fun = eval(str);
}
JSON.parse('[1, 2, [4, 5]]', f);
var ZeNZ = ZeNZ;
var oob = '/re/';
var kwPT = new Map([
    [
        once,
        42,
        a,
        once,
        once
    ],
    [
        once,
        a,
        -5e-324,
        once,
        -4294967297
    ]
]);
