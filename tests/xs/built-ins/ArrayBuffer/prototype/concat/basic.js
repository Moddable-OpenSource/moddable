/*---
description: concat appends the arguments to this
includes: [compareArray.js]
---*/

var a = new Uint8Array([1, 2]).buffer;
var b = new Uint8Array([3]).buffer;
var c = new Uint8Array([4, 5, 6]).buffer;

var dest = a.concat(b, c);
assert.sameValue(dest.byteLength, 6, 'dest.byteLength');
assert.compareArray(Array.from(new Uint8Array(dest)), [1, 2, 3, 4, 5, 6]);

dest = a.concat();
assert.notSameValue(dest, a, 'new instance');
assert.compareArray(Array.from(new Uint8Array(dest)), [1, 2]);

assert.throws(TypeError, function() {
	a.concat({});
});
