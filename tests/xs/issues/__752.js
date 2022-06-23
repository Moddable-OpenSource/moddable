/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/752
flags: [onlyStrict]
---*/

var buf = new ArrayBuffer(49);
var numbers = new Uint8Array(buf);
function v() {
    return 7;
}
function JSEtest(a, b) {
    return { valueOf: v };
}
numbers.sort(JSEtest);
