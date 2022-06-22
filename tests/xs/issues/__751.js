/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/751
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function JSEtest(x, n) {
    while (x.length < n) {
        x += x;
    }
    return x.substring(0, n);
}

var x = JSEtest("1", 1 << 20);
var rep = JSEtest("$1", 1 << 16);
var y = x.replace(/(.+)/g, rep);
y.length;
