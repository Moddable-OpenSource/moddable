/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/766
flags: [onlyStrict]
---*/

var size = 256;
var array1 = new Array(size);

function toStr() {
  array1.splice(0, 2);
  return array1.sort().toString();
}

function JSEtest() {
  for (var i = 0; i < size; i++) {
    array1[i] = new Array(i);
    array1.sort()[i].toString = toStr;
  }
  array1.sort();
}

JSEtest();
