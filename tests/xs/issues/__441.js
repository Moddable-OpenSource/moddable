/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/441
flags: [onlyStrict]
---*/

function test() {

// CreateListFromArrayLike -> Get -> [[Get]]
var get = [];
var p = new Proxy({length:get - 6, 0:0, 1:0}, { get: function(o, k) { get.push(k); return o[k]; }});
Function.prototype.apply({}, p);
return get + '' === "length";
      
}

if (!test())
    throw new Error("Test failed");

