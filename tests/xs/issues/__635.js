/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/635
flags: [onlyStrict]
---*/

let arr = []; 
Number.parseFloat("caller".concat("v1", "value", "-0", /./gimsuy.toString(), Number.prototype.toLocaleString.call("apply".search(/a+b+c/i))));

for (var i = 0; i < 100000; i++) arr[i] = []; 

arr.indexOf(new Object(), {
  valueOf: function () {
    arr.length = 0;
  }
});
