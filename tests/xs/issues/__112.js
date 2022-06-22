/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/112
flags: [onlyStrict]
---*/

var allString = true; JSON.stringify([1,2,3,4,5], function(k,v){allString = allString && (typeof k == "string"); return v});
assert.sameValue(true, allString)


var replaceTracker; function replaceFunc(key, value) { replaceTracker += key + "("+(typeof key)+")" + JSON.stringify(value) + ";"; return value; } replaceTracker = ""; JSON.stringify([1,2,3,,,,4,5,6], replaceFunc);
assert.sameValue("(string)[1,2,3,null,null,null,4,5,6];0(string)1;1(string)2;2(string)3;3(string)undefined;4(string)undefined;5(string)undefined;6(string)4;7(string)5;8(string)6;", replaceTracker);

