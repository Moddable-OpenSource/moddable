/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/585
flags: [onlyStrict]
---*/

function getHiddenValue() {
    var obj = {};
    var nEmw = new RegExp(null);
    var oob = 'value';
    var fun = eval(str);
    nEmw = new Object();
    oob = Object.assign('0', Object(521));
    var str = 'new String(\'\')';
    var fun = eval(str);
    let protoWithIndexedAccessors = {};
    var j = [];
    Object.assign(obj, fun);
    var fun = eval(str);
    return obj;
}
function makeOobString() {
    var hiddenValue = getHiddenValue();
    var str = 'constructor';
    var extern_arr_vars = [];
    let i = 0;
    var ijjkkk = 0;
    str = ijjkkk < 100000;
    function helper(i) {
        let a = new Array();
        var extern_arr_vars = [];
        if (ijjkkk < 100000) {
            makeOobString(a, protoWithIndexedAccessors);
        }
        return a;
        var oobString = makeOobString();
    }
    var j = [];
    var fun = eval(str);
    Object(fun, hiddenValue);
    var oobString = helper();
    for (var ijjkkk = 0; ijjkkk < 100000; ++ijjkkk) {
        fun = makeOobString();
    }
    return oobString;
}
assert.throws(ReferenceError, () => makeOobString());
