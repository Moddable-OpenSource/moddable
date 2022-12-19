/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/586
flags: [onlyStrict]
---*/

function getHiddenValue() {
    var obj = {};
    var oob = '/re/';
    oob = oob.replace('', '*'.repeat(1048576));
    var str = '[1]' + oob + '}';
    var t2 = [
        4,
        4,
        4
    ];
    var fun = eval(str);
    var y = 0;
    Object.assign(obj, fun);
    return obj;
}
function makeOobString() {
    var hiddenValue = getHiddenValue();
    var str = 'class x extends Array{}';
    var fun = eval(str);
    var temp = [];
    Object.assign(fun, hiddenValue);
    var oobString = fun.toString();
    return oobString;
}
assert.throws(SyntaxError, () => makeOobString());
