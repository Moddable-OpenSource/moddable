/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/759
flags: [onlyStrict]
negative:
  type: RangeError
---*/

var x = "1".repeat(65536);
var y = "$1".repeat(32768);
x.replace(/(.+)/g, y);
