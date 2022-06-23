/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/767
flags: [onlyStrict]
---*/

function JSEtest() { };

JSEtest.prototype = Function.prototype;
var obj = new JSEtest;

assert.throws(TypeError, () => obj.apply(), "obj.apply();");
assert.sameValue(obj instanceof JSEtest, true, "obj instanceof JSEtest");
