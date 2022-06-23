/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/582
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function getHiddenValue(){
    var obj = {};
    var oob = "/re/";
    //oob = oob.replace("re","*".repeat(0x2000));
    oob = oob.replace("re",oob = oob.replace("re","*".repeat(0x100000)).repeat(0x100000));
    var str =  'class x extends Array{'+oob+"}";
    var fun = eval(str);
    Object.assign(obj,fun);
    return obj;
}
function makeOobString(){
    var hiddenValue = getHiddenValue();
    var str =  'class x extends Array{}';
    var fun = eval(str);
    Object.assign(fun,hiddenValue);
    var oobString = fun.toString();
    return oobString;
}
makeOobString();

