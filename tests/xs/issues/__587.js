/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/587
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function getHiddenValue() {
    var obj = {};
    var oob = '/re/';
    oob = oob.replace('', '-0'.repeat(1048576));
    var str = '(new Number(-0))' + oob + '(new Boolean(false))';
    var fun = eval(str);
    Object.assign(obj, fun);
    return obj;
}
function makeOobString() {
    var hiddenValue = getHiddenValue();
    var str = '-Infinity';
    var fun = eval(str);
    Object.assign(fun, hiddenValue);
    var oobString = fun.toString();
    return oobString;
}
var oobString = makeOobString();
