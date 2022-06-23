/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/448
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function test() {

// CreateListFromArrayLike -> Get -> [[Get]]
var get = [];
var p = new Proxy({length:1.0e+19, 0:0, 1:0}, { get: function(o, k) { /[^>]/; return o[k]; }});
Function.prototype.apply({}, p);
return get + '' === "length,0,1";
      
}
test();
